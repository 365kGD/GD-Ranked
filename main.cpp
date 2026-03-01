#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/MenuLayer.hpp>
#include <Geode/modify/ProfilePage.hpp>
#include <vector>
#include <string>

using namespace geode::prelude;

// ---------------- GLOBAL DATA ----------------
int g_elo = 0;
std::vector<int> g_recentElo;

// ---------------- SAVE / LOAD ----------------
void saveElo() {
    Mod::get()->setSavedValue("elo", g_elo);
}

void loadElo() {
    g_elo = Mod::get()->getSavedValue<int>("elo", 0);
}

void saveRecentElo() {
    Mod::get()->setSavedValue("recent-elo", g_recentElo);
}

void loadRecentElo() {
    g_recentElo = Mod::get()->getSavedValue<std::vector<int>>("recent-elo", {});
}

// ---------------- RANK SYSTEM ----------------
std::string getRankName(int elo) {
    if (elo <= 100) return "Bronze";
    if (elo <= 500) return "Iron";
    if (elo <= 1000) return "Gold";
    if (elo <= 2500) return "Diamond";
    if (elo <= 5000) return "Demon";
    return "Extreme";
}

// ---------------- ELO LOGIC ----------------
int getElo(GJGameLevel* level) {
    if (!level) return 5;

    if (level->m_demon == 1) {
        switch (level->m_demonDifficulty) {
            case 1: return 50;
            case 2: return 75;
            case 3: return 100;
            case 4: return 250;
            case 5: return 500;
            default: return 50;
        }
    }

    int stars = level->m_stars;
    if (stars == 0) return 0;
    if (stars <= 2) return 5;
    if (stars <= 4) return 10;
    if (stars <= 6) return 15;
    if (stars <= 8) return 25;
    if (stars >= 9) return 40;
    return 5;
}

// ---------------- PLAYLAYER ----------------
class $modify(MyPlayLayer, PlayLayer) {
    void levelComplete() {
        PlayLayer::levelComplete();
        if (!m_level) return;

        int gain = getElo(m_level);
        if (gain <= 0) return;

        g_elo += gain;
        g_recentElo.push_back(gain);
        if (g_recentElo.size() > 10)
            g_recentElo.erase(g_recentElo.begin());

        saveElo();
        saveRecentElo();

        auto winSize = CCDirector::sharedDirector()->getWinSize();
        auto label = CCLabelBMFont::create(
            ("+" + std::to_string(gain) + " ELO").c_str(),
            "bigFont.fnt"
        );
        label->setScale(0.6f);
        label->setPosition({winSize.width / 2, winSize.height / 2 + 40});
        this->addChild(label, 1000);
    }
};

// ---------------- PROFILE PAGE ----------------
class $modify(MyProfilePage, ProfilePage) {
    bool init(int accountID, bool ownProfile) {
        if (!ProfilePage::init(accountID, ownProfile))
            return false;

        loadElo();

        auto eloLabel = CCLabelBMFont::create(
            ("ELO: " + std::to_string(g_elo)).c_str(),
            "bigFont.fnt"
        );
        eloLabel->setScale(0.5f);
        eloLabel->setPosition({450, 283});
        this->addChild(eloLabel);

        return true;
    }
};

// ---------------- RANKED SCENE ----------------
class RankedScene : public CCScene {
    CCLayerColor* contentLayer = nullptr;
    CCMenu* sideMenu = nullptr;

public:
    static RankedScene* create() {
        auto ret = new RankedScene();
        if (ret && ret->init()) {
            ret->autorelease();
            return ret;
        }
        CC_SAFE_DELETE(ret);
        return nullptr;
    }

    bool init() override {
        if (!CCScene::init()) return false;

        loadElo();
        loadRecentElo();

        auto winSize = CCDirector::sharedDirector()->getWinSize();

        contentLayer = CCLayerColor::create({25, 25, 25, 255}, winSize.width, winSize.height);
        this->addChild(contentLayer);

        // Close Button
        auto closeLabel = CCLabelBMFont::create("X", "bigFont.fnt");
        auto closeBtn = CCMenuItemLabel::create(closeLabel, this, menu_selector(RankedScene::onClose));
        auto closeMenu = CCMenu::create(closeBtn, nullptr);
        closeMenu->setPosition({winSize.width - 40, winSize.height - 40});
        this->addChild(closeMenu);

        // Side Menu
        sideMenu = CCMenu::create();
        sideMenu->setPosition({0, 0});
        this->addChild(sideMenu, 1000);

        auto makeBtn = [&](std::string text, SEL_MenuHandler sel, float offsetY) {
            auto lbl = CCLabelBMFont::create(text.c_str(), "bigFont.fnt");
            lbl->setScale(0.5f);
            auto btn = CCMenuItemLabel::create(lbl, this, sel);
            btn->setPosition({winSize.width - 70, winSize.height / 2 + offsetY});
            sideMenu->addChild(btn);
        };

        makeBtn("Leaderboard", menu_selector(RankedScene::showLeaderboard), 40);
        makeBtn("Profile", menu_selector(RankedScene::showProfile), 0);
        makeBtn("Info", menu_selector(RankedScene::showInfo), -40);

        showLeaderboard(nullptr);
        return true;
    }

    void clearContent() {
        contentLayer->removeAllChildren();
    }

    // ---------------- LEADERBOARD ----------------
    void showLeaderboard(CCObject*) {
        clearContent();
        auto winSize = CCDirector::sharedDirector()->getWinSize();
        float centerX = winSize.width / 2;
        float startY = winSize.height - 80;

        auto title = CCLabelBMFont::create("Season 0 - Top 5", "goldFont.fnt");
        title->setScale(0.7f);
        title->setPosition({centerX, startY});
        contentLayer->addChild(title);

        for (int i = 0; i < 5; i++) {
            std::string text = "#" + std::to_string(i + 1) + " N/A - 0";
            auto label = CCLabelBMFont::create(text.c_str(), "chatFont.fnt");
            label->setScale(0.6f);
            label->setPosition({centerX, startY - 50 - i * 35});
            contentLayer->addChild(label);
        }
    }

    // ---------------- PROFILE ----------------
    void showProfile(CCObject*) {
        clearContent();
        auto winSize = CCDirector::sharedDirector()->getWinSize();
        float centerX = 284;

        auto title = CCLabelBMFont::create("Your Profile", "goldFont.fnt");
        title->setScale(0.6f);
        title->setPosition({centerX, 250});
        contentLayer->addChild(title);

        auto eloLabel = CCLabelBMFont::create(("ELO: " + std::to_string(g_elo)).c_str(), "bigFont.fnt");
        eloLabel->setScale(0.7f);
        eloLabel->setPosition({centerX, 200});
        contentLayer->addChild(eloLabel);

        auto rankLabel = CCLabelBMFont::create(("Rank: " + getRankName(g_elo)).c_str(), "goldFont.fnt");
        rankLabel->setScale(0.7f);
        rankLabel->setPosition({centerX, 225});
        contentLayer->addChild(rankLabel);

        // -------- Graph --------
        loadRecentElo();
        size_t graphCount = g_recentElo.size();
        if (graphCount > 10) graphCount = 10;

        if (graphCount >= 2) {
            float spacing = 25.f;
            float totalWidth = (graphCount - 1) * spacing;
            float startX = centerX - totalWidth / 2.f;
            float baseY = 100;

            std::vector<CCPoint> points;
            for (size_t i = 0; i < graphCount; i++) {
                float x = startX + i * spacing;
                float y = baseY + g_recentElo[i] * 0.5f;
                points.push_back({x, y});
            }

            for (size_t i = 0; i < points.size() - 1; i++) {
                auto line = CCDrawNode::create();
                line->drawSegment(points[i], points[i + 1], 2.0f, {0, 1, 0, 1});
                contentLayer->addChild(line);
            }
        }
    }

    // ---------------- INFO ----------------
    void showInfo(CCObject*) {
        clearContent();
        auto winSize = CCDirector::sharedDirector()->getWinSize();
        float centerX = winSize.width / 2;
        float startY = winSize.height / 2 + 80;

        auto title = CCLabelBMFont::create("ELO REWARD SYSTEM", "goldFont.fnt");
        title->setScale(0.5f);
        title->setPosition({centerX, startY});
        contentLayer->addChild(title);

        std::vector<std::string> lines = {
            "Normal Levels",
            "Easy - 5",
            "Normal - 10",
            "Hard - 15",
            "Harder - 25",
            "Insane - 40",
            "",
            "Demon Levels",
            "Easy Demon - 50",
            "Medium Demon - 75",
            "Hard Demon - 100",
            "Insane Demon - 250",
            "Extreme Demon - 500"
        };

        float y = startY - 25;
        for (auto& text : lines) {
            auto label = CCLabelBMFont::create(text.c_str(), "chatFont.fnt");
            label->setScale(0.4f);
            label->setPosition({centerX, y});
            contentLayer->addChild(label);
            y -= 20;
        }
    }

    void onClose(CCObject*) {
        CCDirector::sharedDirector()->popScene();
    }
};

// ---------------- MENU BUTTON ----------------
class $modify(MyMenuLayer, MenuLayer) {
    bool init() {
        if (!MenuLayer::init()) return false;

        auto winSize = CCDirector::sharedDirector()->getWinSize();
        auto menu = CCMenu::create();
        menu->setPosition({0, 0});
        this->addChild(menu);

        auto label = CCLabelBMFont::create("Ranked", "bigFont.fnt");
        label->setScale(0.5f);

        auto btn = CCMenuItemLabel::create(label, this, menu_selector(MyMenuLayer::openRanked));
        btn->setPosition({winSize.width - 80, 60});
        menu->addChild(btn);

        return true;
    }

    void openRanked(CCObject*) {
        CCDirector::sharedDirector()->pushScene(RankedScene::create());
    }
};
