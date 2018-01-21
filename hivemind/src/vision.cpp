#include "stdafx.h"
#include "base.h"
#include "utilities.h"
#include "bot.h"
#include "vision.h"

namespace hivemind {

  HIVE_DECLARE_CONVAR( vision_updateinterval, "Delay between map visibility updates.", 20 );

  Vision::Vision( Bot* bot ): Subsystem( bot )
  {
  }

  void Vision::gameBegin()
  {
    bot_->messaging().listen( Listen_Global | Listen_Intel, this );

    visibility_.resize( bot_->map().width_, bot_->map().height_ );
    visibility_.reset( Visibility_FullHidden );

    lastVisUpdate_ = 0;

    updateVisibility();
  }

  bool Vision::updateVisibility()
  {
    auto& rawData = bot_->observation().GetRawObservation()->raw_data();
    if ( !rawData.has_map_state() || !rawData.map_state().has_visibility() )
      return false;

    visibility_.reset( Visibility_FullHidden );

    auto width = bot_->map().width_;
    auto height = bot_->map().height_;

    auto& regionMap = bot_->map().regionMap_;
    for ( auto region : bot_->map().regions_ )
      regionVision_[region->label_].reset();

    auto& vis = rawData.map_state().visibility().data();
    for ( size_t x = 0; x < width; x++ )
      for ( size_t y = 0; y < height; y++ )
      {
        auto val = (VisibilityTile)vis[x + ( height - 1 - y ) * width];
        visibility_[x][y] = val;
        auto region = regionMap[x][y];
        if ( region >= 0 )
        {
          regionVision_[region].tiles_++;
          if ( val == Visibility_Visible )
            regionVision_[region].visible_++;
        }
      }

    lastVisUpdate_ = bot_->time();

    return true;
  }

  void Vision::update( const GameTime time, const GameTime delta )
  {
    if ( lastVisUpdate_ + g_CVar_vision_updateinterval.as_i() <= time )
    {
      if ( !updateVisibility() )
        bot_->console().printf( "Vision: Map visibility update failed!" );
    }
  }

  void Vision::onMessage( const Message & msg )
  {
    if ( msg.code == M_Intel_FoundPlayer )
    {
      for ( auto& locId : bot_->vision().startLocations().unexplored_ )
        if ( bot_->map().baseLocations_[locId].containsPosition( msg.unit( 1 )->pos ) )
        {
          bot_->vision().startLocations().unexplored_.erase( locId );
          bot_->vision().startLocations().explored_.insert( locId );
          break;
        }
    }
  }

  void Vision::gameEnd()
  {
    bot_->messaging().remove( this );
  }

}