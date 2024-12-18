#ifndef EVENTPICK_H
#define EVENTPICK_H

#include <iostream>
#include "SkimTree.h"
#include "GlobalFlag.h"

class EventPick{
public:
    // Constructor accepting a reference to GlobalFlag
    explicit EventPick(GlobalFlag& globalFlags);

    bool passFilter(const std::shared_ptr<SkimTree>& tree) const;
    bool passHLT(const std::shared_ptr<SkimTree>& tree) const;
    
    std::vector<std::string> getTrigNames() const;
    std::map<std::string, const Bool_t*> getTrigValues() const;

    ~EventPick();

private:
    // Reference to GlobalFlag instance
    GlobalFlag& globalFlags_;
    const GlobalFlag::Year year_;
    const GlobalFlag::Channel channel_;
    const bool isDebug_;
    const bool isMC_;
    void printDebug(const std::string& message) const;
};

#endif // EVENTPICK_H

