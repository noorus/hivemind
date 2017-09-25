#include "stdafx.h"
#include "ai_goals.h"
#include "exception.h"
#include "brain.h"
#include "strategy.h"
#include "utilities.h"
#include "bot.h"

namespace hivemind {

  void debugDrawFuzzyVariable( AI::FuzzyVariable& var, Point2D screenPosition, sc2::DebugInterface& dbg )
  {
    const Point2D increment( 0.005f, 0.01f );
    dbg.DebugTextOut( string( "FuzzyVariable " ) + var.name(), screenPosition, sc2::Colors::White );
    screenPosition.y += increment.y;

    for ( auto& set : var.members() )
    {
      char text[128];
      sprintf_s( text, 128, "%s: %.3f", set.first.c_str(), set.second->dom() );
      dbg.DebugTextOut( text, screenPosition, sc2::Colors::Teal );
      screenPosition.y += increment.y;
    }
  }

  Strategy::Strategy( Bot* bot ): Subsystem( bot ), fzGameTime_( nullptr )
  {
  }

  Strategy::~Strategy()
  {
  }

  const size_t c_time3Minutes   = utils::timeToTicks( 3, 0 );
  const size_t c_time6Minutes   = utils::timeToTicks( 6, 0 );
  const size_t c_time9Minutes   = utils::timeToTicks( 9, 0 );
  const size_t c_time12Minutes  = utils::timeToTicks( 12, 0 );
  const size_t c_time15Minutes  = utils::timeToTicks( 15, 0 );
  const size_t c_time2Hours     = utils::timeToTicks( 2 * 60, 0 );

  void Strategy::gameBegin()
  {
    strats_.push_back( new Strategy_Economic( bot_ ) );
    strats_.push_back( new Strategy_Aggressive( bot_ ) );
    strats_.push_back( new Strategy_Defensive( bot_ ) );

    fzGameTime_ = &fuzzyMod_.createVariable( "GameTime" );
    fzGameTime_->addLeftShoulder( "Time_Early", 0.0f, (Real)c_time3Minutes, (Real)c_time6Minutes ); // 0-3, -> 3-6
    fzGameTime_->addTrapezoid( "Time_Mid", (Real)c_time3Minutes, (Real)c_time6Minutes, (Real)c_time12Minutes, (Real)c_time15Minutes ); // 3-6-12-15
    fzGameTime_->addRightShoulder( "Time_Late", (Real)c_time12Minutes, (Real)c_time15Minutes, (Real)c_time2Hours ); // 12-15 -> +
  }

  void Strategy::update( const GameTime time, const GameTime delta )
  {
    fuzzyMod_.fuzzify( "GameTime", (Real)bot_->time() );
  }

  void Strategy::draw()
  {
    if ( fzGameTime_ )
      debugDrawFuzzyVariable( *fzGameTime_, Point2D( 0.03f, 0.3f ), bot_->debug() );
  }

  void Strategy::gameEnd()
  {
    for ( auto& strat : strats_ )
      SAFE_DELETE( strat );

    fzGameTime_ = nullptr;
  }

  Strategy_Base::Strategy_Base( Bot* bot ): bot_( bot )
  {
  }

  Strategy_Economic::Strategy_Economic( Bot* bot ): Strategy_Base( bot )
  {
  }

  Strategy_Aggressive::Strategy_Aggressive( Bot* bot ): Strategy_Base( bot )
  {
  }

  Strategy_Defensive::Strategy_Defensive( Bot* bot ): Strategy_Base( bot )
  {
  }

}