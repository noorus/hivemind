#include "stdafx.h"
#include "base.h"
#include "utilities.h"
#include "bot.h"
#include "intelligence.h"
#include "database.h"
#include "pathing.h"
#include "pathfinding.h"

namespace hivemind {

  Path::Path( Pathing* pathing ): host_( pathing )
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
    pathfinding::Dstar dstar;
    list<pathfinding::state> mypath;

    dstar.init( from.x, from.y, to.x, to.y );

    PathPtr path = std::make_shared<Path>( this );
    graph_->reset();

    graph_->applyTo( &dstar );
    dstar.replan();
    mypath = dstar.getPath();

    vector<Vector2> verts;
    for ( auto& pt : mypath )
      verts.push_back( Vector2( pt.x, pt.y ) );

    //auto mapPath = pathfinding::pathAStarSearch( *(graph_.get()), from, to );
    //auto clipperPath = util_contourToClipperPath( mapPath );
    //ClipperLib::CleanPolygon( clipperPath );
    //auto poly = util_clipperPathToPolygon( clipperPath );
    //vector<Vector2> verts;
    //for ( auto& pt : mapPath )
    //  verts.push_back( pt );
    path->assignVertices( verts );
    paths_.push_back( path );
    return path;
  }

  void Pathing::clear()
  {
    paths_.clear();
  }

  void Pathing::gameBegin()
  {
    bot_->messaging().listen( Listen_Global, this );
    graph_ = std::make_unique<pathfinding::GridGraph>( bot_->map() );
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
      for ( const auto& v : path->verts() )
      {
        if ( previous.x > 0.0f && previous.y > 0.0f )
        {
          bot_->debug().drawMapLine( bot_->map(), previous, v, color );
        }
        bot_->debug().drawMapSphere( bot_->map(), v, 0.25f, color );
        previous = v;
      }
      ++i;
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