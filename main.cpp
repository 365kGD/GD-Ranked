#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/MenuLayer.hpp>
#include <Geode/modify/ProfilePage.hpp>
#include <vector>
#include <string>

using namespace geode::prelude;

int g_elo = 0;
std::vector<int> g_recentElo;

void saveElo() { Mod::get()->setSavedValue<int>("elo", g_elo); }
void loadElo() { g_elo = Mod::get()->getSavedValue<int>("elo", 0); }

int getElo(GJGameLevel* level) {
    if (!level) return 5;
    int diff = static_cast<int>(level->m_difficulty);
    int demon = level->m_demonDifficulty;
    if(diff == 6 || demon > 0){
        switch(demon){
            case 3: return 50;
            case 4: return 75;
            case 5: return 100;
            case 6: return 250;
            case 7: return 500;
            default: return 50;
        }
    }
    switch(diff){
        case 1: return 5;
        case 2: return 10;
        case 3: return 15;
        case 4: return 25;
        case 5: return 40;
        default: return 10;
    }
}

class $modify(MyPlayLayer, PlayLayer) {
    void levelComplete() {
        PlayLayer::levelComplete();
        if(!m_level) return;
        int gain = getElo(m_level);
        g_elo += gain;
        g_recentElo.push_back(gain);
        saveElo();
        auto winSize = CCDirector::sharedDirector()->getWinSize();
        auto label = CCLabelBMFont::create(("+" + std::to_string(gain) + " ELO").c_str(), "bigFont.fnt");
        label->setScale(0.6f);
        label->setPosition(ccp(winSize.width/2, winSize.height/2 + 40));
        this->addChild(label, 1000);
    }
};

class $modify(MyProfilePage, ProfilePage) {
    bool init(int accountID, bool ownProfile) {
        if(!ProfilePage::init(accountID, ownProfile)) return false;
        loadElo();
        auto winSize = CCDirector::sharedDirector()->getWinSize();
        auto eloLabel = CCLabelBMFont::create(("ELO: " + std::to_string(g_elo)).c_str(), "bigFont.fnt");
        eloLabel->setScale(0.5f);
        eloLabel->setPosition(ccp(450, 283));
        this->addChild(eloLabel);
        return true;
    }
};

struct TopPlayer { std::string name; int elo; };

class RankedScene : public CCScene {
    CCLayerColor* contentLayer = nullptr;
    std::vector<TopPlayer> localTop5;
    CCMenu* sideMenu = nullptr;

public:
    static RankedScene* create() {
        auto ret = new RankedScene();
        if(ret && ret->init()){ ret->autorelease(); return ret; }
        CC_SAFE_DELETE(ret);
        return nullptr;
    }

    bool init() override {
        if(!CCScene::init()) return false;
        auto winSize = CCDirector::sharedDirector()->getWinSize();
        contentLayer = CCLayerColor::create({25,25,25,255}, winSize.width, winSize.height);
        this->addChild(contentLayer);
        auto closeLabel = CCLabelBMFont::create("X", "bigFont.fnt");
        auto closeBtn = CCMenuItemLabel::create(closeLabel, this, menu_selector(RankedScene::onClose));
        auto closeMenu = CCMenu::create(closeBtn, nullptr);
        closeMenu->setPosition(ccp(winSize.width-40, winSize.height-40));
        this->addChild(closeMenu);
        sideMenu = CCMenu::create();
        sideMenu->setPosition({0,0});
        this->addChild(sideMenu, 1000);

        auto makeBtn = [&](std::string text, SEL_MenuHandler sel, float offsetY){
            auto lbl = CCLabelBMFont::create(text.c_str(), "bigFont.fnt");
            lbl->setScale(0.5f);
            auto btn = CCMenuItemLabel::create(lbl, this, sel);
            btn->setPosition(ccp(winSize.width - 60, winSize.height/2 + offsetY));
            sideMenu->addChild(btn);
        };

        makeBtn("Global", menu_selector(RankedScene::showGlobal), 60);
        makeBtn("Profile", menu_selector(RankedScene::showProfile), 20);
        makeBtn("Info", menu_selector(RankedScene::showInfo), -20);

        localTop5 = { {"Player1", 2000}, {"Player2", 1950}, {"Player3", 1900}, {"Player4", 1850}, {"Player5", 1800} };

        showGlobal(nullptr);
        return true;
    }

    void clearContent(){ contentLayer->removeAllChildren(); }

    void showGlobal(CCObject*){
        clearContent();
        auto winSize = CCDirector::sharedDirector()->getWinSize();
        float startY = winSize.height - 80;
        auto seasonLabel = CCLabelBMFont::create("Season 0 - Top 5", "goldFont.fnt");
        seasonLabel->setScale(0.6f);
        seasonLabel->setPosition(ccp(winSize.width/2, startY));
        contentLayer->addChild(seasonLabel);

        for(int i=0;i<5;i++){
            std::string text = "#" + std::to_string(i+1) + " " + localTop5[i].name + " - " + std::to_string(localTop5[i].elo);
            auto label = CCLabelBMFont::create(text.c_str(), "chatFont.fnt");
            label->setScale(0.6f);
            label->setPosition(ccp(winSize.width/2, startY - 40 - i*30));
            contentLayer->addChild(label);
        }
    }

    void showProfile(CCObject*){
        clearContent();
        auto winSize = CCDirector::sharedDirector()->getWinSize();
        auto graphLabel = CCLabelBMFont::create("ELO Progress Graph", "bigFont.fnt");
        graphLabel->setScale(0.6f);
        graphLabel->setPosition(ccp(winSize.width/2, winSize.height/2 + 80));
        contentLayer->addChild(graphLabel);

        auto label = CCLabelBMFont::create(("Your ELO: " + std::to_string(g_elo)).c_str(), "bigFont.fnt");
        label->setScale(0.7f);
        label->setPosition(ccp(winSize.width/2, winSize.height/2 + 40));
        contentLayer->addChild(label);

        float startX = winSize.width/2 - g_recentElo.size()*10/2.0f;
        float startY = winSize.height/2 - 20;
        for(size_t i=0;i<g_recentElo.size();i++){
            auto bar = CCLayerColor::create({50,200,50,255}, 10, g_recentElo[i]*0.5f);
            bar->setPosition(ccp(startX + i*12, startY));
            contentLayer->addChild(bar);
        }
    }

    void showInfo(CCObject*){
        clearContent();
        auto winSize = CCDirector::sharedDirector()->getWinSize();
        std::string infoText = "ELO PER LEVEL:\nEasy:5\nNormal:10\nHard:15\nHarder:25\nInsane:40\nDemon Levels:\nEasy Demon:50\nMedium Demon:75\nHard Demon:100\nInsane Demon:250\nExtreme Demon:500";
        auto label = CCLabelBMFont::create(infoText.c_str(), "bigFont.fnt");
        label->setScale(0.5f);
        label->setPosition(ccp(winSize.width/2, winSize.height/2 + 50));
        contentLayer->addChild(label);
    }

    void onClose(CCObject*){ CCDirector::sharedDirector()->popScene(); }
};

class $modify(MyMenuLayer, MenuLayer){
    bool init(){
        if(!MenuLayer::init()) return false;
        auto winSize = CCDirector::sharedDirector()->getWinSize();
        auto menu = CCMenu::create();
        menu->setPosition({0,0});
        this->addChild(menu);
        auto label = CCLabelBMFont::create("Ranked", "bigFont.fnt");
        label->setScale(0.5f);
        auto btn = CCMenuItemLabel::create(label, this, menu_selector(MyMenuLayer::openRanked));
        btn->setPosition(ccp(winSize.width - 80, 60));
        menu->addChild(btn);
        return true;
    }

    void openRanked(CCObject*){ CCDirector::sharedDirector()->pushScene(RankedScene::create()); }
};