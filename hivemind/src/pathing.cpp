#include "stdafx.h"
#include "base.h"
#include "utilities.h"
#include "bot.h"
#include "intelligence.h"
#include "database.h"
#include "pathing.h"
#include "pathfinding.h"

namespace hivemind {

  HIVE_DECLARE_CONVAR( draw_paths, "Whether to draw debug information about pathfinding. 0 = none, 1 = basic, 2 = verbose", 1 );

  Path::Path( Pathing* pathing ):
    host_( pathing )
  {
  }

  void Path::assignVertices( const vector<Vector2>& path )
  {
    verts_ = path;
  }

  Path::~Path()
  {
  }

  Pathing::Pathing( Bot* bot ): Subsystem( bot )
  {
  }

  PathPtr Pathing::createPath( const Vector2 & from, const Vector2 & to )
  {
    PathPtr path = std::make_shared<Path>( this );

    auto dstarResult = pathfinding::DStarLite::search( &bot_->console(), std::make_unique<pathfinding::GridMapAdaptor>( &bot_->map() ), from, to );
    auto mapPath = dstarResult->getMapPath();

    auto clipperPath = util_contourToClipperPath( mapPath );
    ClipperLib::CleanPolygon( clipperPath );
    auto poly = util_clipperPathToPolygon( clipperPath );
    vector<Vector2> verts;
    for ( auto& pt : mapPath )
      verts.push_back( pt );
    path->assignVertices( verts );

    path->dstarResult = std::move(dstarResult);

    paths_.push_back( path );
    return path;
  }

  void Pathing::updatePathWalkability(MapPoint2 changedNode, bool hasObstacle)
  {
    //bot_->console().printf("Marking pathing obstacle=%d at (%d, %d)", hasObstacle, changedNode.x, changedNode.y);

    for(auto& path : paths_)
    {
      path->dstarResult->updateWalkability(changedNode, hasObstacle);
    }
  }

  void Pathing::updatePaths()
  {
    for(auto& path : paths_)
    {
      path->dstarResult->computeShortestPath();

      auto mapPath = path->dstarResult->getMapPath();
      auto clipperPath = util_contourToClipperPath( mapPath );
      ClipperLib::CleanPolygon( clipperPath );
      auto poly = util_clipperPathToPolygon( clipperPath );
      vector<Vector2> verts;
      for ( auto& pt : mapPath )
        verts.push_back( pt );
      path->assignVertices( verts );
    }
  }

  void Pathing::clear()
  {
    paths_.clear();
  }

  void Pathing::gameBegin()
  {
    bot_->messaging().listen( Listen_Global, this );
    graph_ = std::make_unique<pathfinding::GridGraph>(&bot_->console(), std::make_unique<pathfinding::GridMapAdaptor>( &bot_->map() ));
    graph_->initialize();
  }

  void Pathing::update(const GameTime time, const GameTime delta)
  {
    platform::PerformanceTimer timer;
    timer.start();

    bool hasChanges = false;

    auto& map = bot_->map();
    for(int x = 0; x < map.width(); ++x)
    {
      for(int y = 0; y < map.height(); ++y)
      {
        bool blockStatus = map.isBlocked(x, y);
        auto& node = graph_->node(x, y);
        if(blockStatus != node.hasObstacle)
        {
          node.hasObstacle = blockStatus;

          updatePathWalkability({ x, y }, blockStatus);

          hasChanges = true;
        }
      }
    }

    if(hasChanges)
    {
      updatePaths();
    }

    double updateDuration = timer.stop();
    if(hasChanges)
    {
      bot_->console().printf("Pathing update took %f ms", updateDuration);
    }
  }

  void Pathing::draw()
  {
    if(g_CVar_draw_paths.as_i() < 1)
      return;

    int i = 1;
    for ( auto& path : paths_ )
    {
      auto color = utils::prettyColor( i, 25 );
      Vector2 previous;
      for ( auto v : path->verts() )
      {
        v -= sc2::Point3D(0.5f, 0.5f, 0.0f); // Prevent double adjust: there is one in conversion from MapPoint2 and one in the drawing function.

        if ( previous.x > 0.0f && previous.y > 0.0f )
        {
          bot_->debug().drawMapLine( bot_->map(), previous, v, color );
        }
        bot_->debug().drawMapSphere( bot_->map(), v, 0.25f, color );
        previous = v;
      }
      ++i;

      if(g_CVar_draw_paths.as_i() < 2)
        continue;

      for ( int y = 0; y < path->dstarResult->graph_->height_; ++y )
      {
        for ( int x = 0; x < path->dstarResult->graph_->width_; ++x )
        {
          const auto& node = path->dstarResult->graph_->node({ x, y });

          Real rhs = (Real)node.rhs;
          Real g = (Real)node.g;

#if HIVE_PATHING_USE_FIXED_POINT
          rhs /= 1000.0f;
          g /= 1000.0f;
          if(node.rhs == std::numeric_limits<int>::max() && node.rhs == node.g)
            continue;
#endif
          if(rhs > 10000.0f)
            rhs /= 10000.0f;
          if(g > 10000.0f)
            g /= 10000.0f;

          if(rhs < 1000.0f || g < 1000.0f)
          {
            auto color = Colors::Green;
            if(rhs > g)
              color = Colors::Red;
            if(rhs < g)
              color = Colors::Yellow;

            auto pos = sc2::Point3D( float( x ), float( y ), 0.0f );
            pos.z = bot_->map().heightMap_[x][y] + 0.21f;

            bot_->debug().drawBox( pos, pos + sc2::Point3D( 1.0f, 1.0f, 0.0f ), color );

            stringstream ss;
            ss << std::fixed << std::setprecision(1) << rhs << "/" << g;

            pos.x -= 0.25f;
            bot_->debug().drawText( ss.str(), Vector3( pos + sc2::Point3D( 0.5f, 0.5f, 0.0f ) ), color );
          }
        }
      }
    }
  }

  void Pathing::onMessage( const Message& msg )
  {
  }

  void Pathing::gameEnd()
  {
    bot_->messaging().remove( this );
    graph_.reset( nullptr );
  }

}
