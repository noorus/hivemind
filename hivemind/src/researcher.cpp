#include "stdafx.h"
#include "base.h"
#include "utilities.h"
#include "bot.h"
#include "Researcher.h"
#include "database.h"
#include "controllers.h"

namespace hivemind {

  HIVE_DECLARE_CONVAR( researcher_debug, "Whether to show and print debug information on the Researcher subsystem. 0 = none, 1 = basics, 2 = verbose", 1 );

  Researcher::Researcher(Bot* bot, BuildProjectID& idPool, std::unordered_map<sc2::UPGRADE_ID, UpgradeStatus>& upgradeStats) :
      Subsystem(bot),
      idPool_(idPool),
      upgradeStats_(upgradeStats)
  {
  }

  void Researcher::gameBegin()
  {
    researchProjects_.clear();
    bot_->messaging().listen(Listen_Global, this);
  }

  UnitRef Researcher::getResearcher(Base& base, UnitTypeID researcherType) const
  {
    for(auto building : base.buildings())
    {
      if(building->unit_type != researcherType)
        continue;
      if(building->build_progress < 1.0f)
        continue;

      if(!building->orders.empty())
        continue;
      if(researchers_.count(building) > 0)
        continue;

      return building;
    }

    return nullptr;
  }

  bool Researcher::research( UpgradeID upgradeType, Base* base, UnitTypeID researcherType, BuildProjectID& idOut )
  {
    auto researcher = getResearcher(*base, researcherType);

    if(!researcher)
    {
      return false;
    }

    Research research( idPool_++, upgradeType, researcherType, researcher );

    researchProjects_.push_back( research );

    if ( g_CVar_researcher_debug.as_i() > 1 )
      bot_->console().printf( "Researcher: New ResearchOp %d for %s", research.id, sc2::UpgradeIDToName( upgradeType ) );

    researchers_.insert(researcher);

    assert(upgradeStats_[upgradeType] == UpgradeStatus::NotStarted);
    upgradeStats_[upgradeType] = UpgradeStatus::inProgress;

    idOut = research.id;
    return true;
  }

  void Researcher::remove( BuildProjectID id )
  {
    for(auto& research : researchProjects_)
    {
      if(research.id == id)
      {
        research.cancel = true;
      }
    }
  }

  void Researcher::gameEnd()
  {
    bot_->messaging().remove( this );
  }

  void Researcher::onMessage( const Message& msg )
  {
    auto researchOver = [this](auto it)
    {
      auto research = *it;

      if ( g_CVar_researcher_debug.as_i() > 1 )
        bot_->console().printf( "Researcher: Removing ResearchOp %d (%s)", research.id, research.completed ? "completed" : "canceled" );

      researchers_.erase(research.researcher);
      auto& stats = upgradeStats_[research.upgradeType];
      if(stats != UpgradeStatus::Researched)
        stats = UpgradeStatus::NotStarted;
      researchProjects_.erase(it);
    };

    if ( msg.code == M_Global_UpgradeCompleted )
    {
      UpgradeID upgrade = msg.upgrade();

      upgradeStats_[upgrade] = UpgradeStatus::Researched;

      for(auto it = researchProjects_.begin(); it != researchProjects_.end(); ++it)
      {
        auto& research = *it;
        if(research.upgradeType == upgrade)
        {
          research.completed = true;

          researchOver(it);
          break;
        }
      }
    }
    else if(msg.code == M_Global_UnitDestroyed)
    {
      UnitRef unit = msg.unit();

      if(!utils::isMine(unit))
      {
        return;
      }

      for(auto it = researchProjects_.begin(); it != researchProjects_.end(); ++it)
      {
        auto& research = *it;
        if(research.researcher == unit)
        {
          research.cancel = true;

          researchOver(it);
          break;
        }
      }
    }
  }

  void Researcher::update( const GameTime time, const GameTime delta )
  {
    auto verbose = ( g_CVar_researcher_debug.as_i() > 1 );

    const GameTime cResearchRecheckInterval = 50;

    for(auto& research : researchProjects_)
    {
      if(!research.moneyAllocated)
      {
        if(research.researcher && research.researcher->is_alive && !research.researcher->orders.empty())
        {
          research.moneyAllocated = true;

          if ( verbose )
            bot_->console().printf( "ResearchOp %d: research started with %x", research.id, id( research.researcher ) );
        }
        else if ( research.nextUpdateTime <= time )
        {
          research.nextUpdateTime = time + cResearchRecheckInterval;

          if(research.researcher && research.researcher->is_alive && research.researcher->orders.empty())
          {
            bot_->unitDebugMsgs_[research.researcher] = "Researcher, Op " + std::to_string(research.id);

            if ( verbose )
              bot_->console().printf( "ResearchOp %d: Got Researcher %x for %s", research.id, id( research.researcher ), sc2::UpgradeIDToName( research.upgradeType ) );


            auto ability = Database::techTree().getUpgradeInfo( research.upgradeType, research.researcher->unit_type ).ability;
            bot_->action().UnitCommand( research.researcher, ability );

            research.tries++;
          }
          else
          {
            // TODO: Get new researcher?
          }
        }
      }
    }
  }

  std::pair<int,int> Researcher::getAllocatedResources() const
  {
    int mineralSum = 0;
    int vespeneSum = 0;
    for(auto& research : researchProjects_)
    {
      if(research.moneyAllocated)
      {
        continue;
      }

      const auto& data = Database::units().at(research.upgradeType);
      mineralSum += data.mineralCost;
      vespeneSum += data.vespeneCost;
    }
    return { mineralSum, vespeneSum };
  }

}
