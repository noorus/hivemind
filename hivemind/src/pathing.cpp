#include "stdafx.h"
#include "base.h"
#include "utilities.h"
#include "bot.h"
#include "intelligence.h"
#include "database.h"
#include "pathing.h"
#include "pathfinding.h"

namespace hivemind {

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

    //auto mapPath = pathfinding::pathAStarSearch( *(graph_.get()), from, to );

    auto dstarResult = pathfinding::DStarLite::search( *(graph_.get()), from, to );
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

  void Pathing::updatePaths(MapPoint2 obstacle)
  {
    for(auto& path : paths_)
    {
      updatePath(path, obstacle);
    }
  }

  void Pathing::updatePath(PathPtr path, MapPoint2 obstacle)
  {
    path->dstarResult->update(obstacle);

    auto mapPath = path->dstarResult->getMapPath();
    auto clipperPath = util_contourToClipperPath( mapPath );
    ClipperLib::CleanPolygon( clipperPath );
    auto poly = util_clipperPathToPolygon( clipperPath );
    vector<Vector2> verts;
    for ( auto& pt : mapPath )
      verts.push_back( pt );
    path->assignVertices( verts );
  }

  void Pathing::clear()
  {
    paths_.clear();
  }

  void Pathing::gameBegin()
  {
    bot_->messaging().listen( Listen_Global, this );
    graph_ = std::make_unique<pathfinding::GridGraph>( bot_, bot_->map() );
    graph_->process();
  }

  void Pathing::update( const GameTime time, const GameTime delta )
  {
  }

  void Pathing::draw()
  {
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
    }

    for ( int y = 0; y < graph_->height_; ++y )
    {
      for ( int x = 0; x < graph_->width_; ++x )
      {
        const auto& node = graph_->node({ x, y });

        if(node.rhs < 1000.0f || node.g < 1000.0f)
        {
          auto color = Colors::White;

          auto pos = sc2::Point3D( float( x ), float( y ), 0.0f );
          pos.z = bot_->map().heightMap_[x][y] + 0.2f;

          bot_->debug().drawBox( pos, pos + sc2::Point3D( 1.0f, 1.0f, 0.0f ), color );

          stringstream ss;
          ss << std::fixed << std::setprecision(1) << node.rhs << "/" << node.g;

          pos.x -= 0.25f;
          bot_->debug().drawText( ss.str(), Vector3( pos + sc2::Point3D( 0.5f, 0.5f, 0.0f ) ), color );
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
