#pragma once
#include "sc2_forward.h"
#include "subsystem.h"
#include "player.h"
#include "baselocation.h"
#include "messaging.h"

namespace hivemind {

  class Pathing;

  namespace pathfinding {
    class GridGraph;
    class DStarLite;
    typedef std::unique_ptr<DStarLite> DStarLitePtr;
  }

  class Path
  {
  private:
    Pathing* host_;
    vector<Vector2> verts_;

  public:
    explicit Path( Pathing* pathing );
    void assignVertices( const vector<Vector2>& path );
    inline const vector<Vector2>& verts() const { return verts_; }
    ~Path();

    pathfinding::DStarLitePtr dstarResult;
  };

  using PathPtr = std::shared_ptr<Path>;

  using PathList = list<PathPtr>;

  struct CachedPath {
  public:
    vector<Vector2> vertices_;
    Real length_;
    CachedPath( const Path& source ): length_( 0.0f )
    {
      vertices_ = source.verts();
      if ( !vertices_.empty() )
      {
        auto previous = vertices_[0];
        for ( auto& vert : vertices_ )
        {
          length_ += previous.distance( vert );
        }
      }
    }
  };

  class Pathing : public Subsystem, Listener
  {
  private:
    PathList paths_;
    std::unique_ptr<pathfinding::GridGraph> graph_;

    void updatePaths();
    void updatePathWalkability( MapPoint2 changedNode, bool hasObstacle );

  public:
    explicit Pathing( Bot* bot );

    PathPtr createPath( const Vector2& from, const Vector2& to );
    void destroyPath( PathPtr& path ); // Removes path from internal update list, but the object is still usable.

    void clear();
    void gameBegin() final;
    void update( const GameTime time, const GameTime delta );
    void draw() final;
    void onMessage( const Message& msg ) final;
    void gameEnd() final;
  };
}