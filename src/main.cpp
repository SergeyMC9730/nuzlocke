/**
 * Include the Geode headers.
 */
#include <Geode/Geode.hpp>

/**
 * Brings cocos2d and all Geode namespaces to the current scope.
 */
using namespace geode::prelude;

#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/GJGarageLayer.hpp>
#include <Geode/binding/CCCircleWave.hpp>
#include <Geode/cocos/particle_nodes/CCParticleSystemQuad.h>
#include <Geode/binding/FMODAudioEngine.hpp>
#include <Geode/modify/PauseLayer.hpp>
#include <Geode/ui/TextInput.hpp>
#include <Geode/modify/MenuLayer.hpp>
#include <Geode/binding/GJGameLevel.hpp>
#include <Geode/binding/GJGameLoadingLayer.hpp>

#include "json/single_include/nlohmann/json.hpp"

#include <vector>
#include <unordered_map>
#include <istream>
#include <filesystem>

class NGJGarageLayer;

class NewAccountProtocol : public FLAlertLayerProtocol {
public:
	void FLAlert_Clicked(FLAlertLayer *p0, bool p1) {
		// if (p1) return FLAlertLayerProtocol::FLAlert_Clicked(p0, p1);

		// log::info("CREATING SCENE");

		// CCScene *scene = CCScene::get();

		// log::info("CREATING ACCOUNTHELPLAYER");
		// AccountHelpLayer *l = new AccountHelpLayer();

		// log::info("b");
		// l->init("Account Help");
		// log::info("c");
		// l->autorelease();
		// log::info("d");
		// l->customSetup();

		// auto help = l;

		// // auto help = AccountHelpLayer::create();

		// log::info("ADDING HELP LAYER");
		// scene->addChild(help, 1000);

		// log::info("SHOWING HELP LAYER");
		// help->showLayer(true);

		FLAlertLayerProtocol::FLAlert_Clicked(p0, p1);
	}
};

#include <functional>

class GiveUpProtocol : public FLAlertLayerProtocol {
private:
	bool _outsideGarage = false;

	std::vector<int> *_deadIcons;
	std::unordered_map<int, std::string> *_iconNames;

	std::function<void(void)> _saveFunc;
public:
	GiveUpProtocol(std::vector<int> *deadIcons, std::unordered_map<int, std::string> *iconNames, std::function<void(void)> saveFunc) {
		_deadIcons = deadIcons;
		_iconNames = iconNames;
		_saveFunc = saveFunc;
	}

	void resetNuzlocke() {
		_deadIcons->clear();
		_iconNames->clear();

		_saveFunc();
	}

	void FLAlert_Clicked(FLAlertLayer *p0, bool p1) {
		FLAlertLayerProtocol::FLAlert_Clicked(p0, p1);

		if (p1 == true) {
			resetNuzlocke();

			if (_outsideGarage) {
				FLAlertLayer::create("Nuzlocke", "Nuzlocke Challenge progress <cr>has been reseted.</c>", "OK")->show();
			} else {
				auto scene = GJGarageLayer::scene();
				auto transition = CCTransitionFade::create(0.5f, scene);

				CCDirector::sharedDirector()->replaceScene(transition);
			}
		}
	}

	void setupOutsideGarage() {
		_outsideGarage = true;
	}
	void setupInsideGarage() {
		_outsideGarage = false;
	}
};

namespace NGlobal {
	bool garageDestroyIcon = false;
	bool garageCountIcons = false;
	int destroyID = 0;
	std::vector<int> deadIcons = {};
	std::unordered_map<int, std::string> iconNames = {};
	bool garageBeginIconSelect = false;
	NGJGarageLayer *currentGarage;
	bool newAccountPopupShown = false;
	NewAccountProtocol popup;
	GiveUpProtocol *giveUp;
	GJGameLevel *loadedLevel;
	GJGameLevel *loadedLevelL;

	void save() {
		nlohmann::json main;

		main["dead-icons"] = deadIcons;
		main["icon-data"] = nlohmann::json();
		main["icon-data"]["names"] = iconNames;
		main["new-account-popup-shown"] = newAccountPopupShown;

		std::string text = main.dump();

		std::string path = Mod::get()->getSaveDir().generic_string();
		std::string filename = fmt::format("{}/progress.json", path);

		std::ofstream o(filename);

		o << text;

		o.close();
	}
	void recover() {
		std::string path = Mod::get()->getSaveDir().generic_string();
		std::string filename = fmt::format("{}/progress.json", path);

		if (!std::filesystem::exists(filename)) return;

		std::ifstream t(filename);
		std::stringstream buffer;

		buffer << t.rdbuf();

		t.close();

		nlohmann::json main = nlohmann::json::parse(buffer.str());

		if (main.contains("dead-icons")) {
			nlohmann::json di_array = main["dead-icons"];

			if (di_array.is_array()) {
				for (int val : di_array) {
					deadIcons.push_back(val);
				}
			}
		}

		if (main.contains("icon-data")) {
			if (main["icon-data"].contains("names")) {
				nlohmann::json narray = main["icon-data"]["names"];

				for (nlohmann::json arr_el : narray) {
					if (arr_el.is_array() && arr_el.size() >= 2) {
						iconNames[arr_el[0].get<int>()] = arr_el[1].get<std::string>();
					}
				}
			}
		}

		if (main.contains("new-account-popup-shown")) {
			newAccountPopupShown = main["new-account-popup-shown"].get<bool>();
		}
	}

	void setupProtocols() {
		giveUp = new GiveUpProtocol(&deadIcons, &iconNames, NGlobal::save);
	}

	void printNewIconError() {
		FLAlertLayer::create(giveUp, "Nuzlocke", "You have to <cy>choose new icon</c> before playing levels.", "OK", "Give Up")->show();
	}
	void printDeadIconError() {
		FLAlertLayer::create(giveUp, "Nuzlocke", "You <cr>cannot</c> play levels with <cy>killed icon</c>.", "OK", "Give Up")->show();
	}
	void printResetPopup() {
		FLAlertLayer::create(giveUp, "Nuzlocke", "Are you sure you want to <cy>reset Nuzlocke progress</c>?", "No", "Yes")->show();
	}

	GJGameLevel *copyLevel(GJGameLevel *_lvl) {
		DS_Dictionary *dict = new DS_Dictionary();

		_lvl->encodeWithCoder(dict);

		// GJGameLevel *lvl = (GJGameLevel *)GJGameLevel::createWithCoder(dict);
		// lvl->retain();

		GJGameLevel *lvl = GJGameLevel::create();
		lvl->dataLoaded(dict);
		lvl->retain();

		lvl->m_levelString = _lvl->m_levelString;

		delete dict;

		log::info("copied level name : {}", lvl->m_levelName);
		// log::info("copied level string: {}", lvl->m_levelString);

		return lvl;
	}

	void replaceScene(CCScene *scene) {
		auto transition = CCTransitionFade::create(0.5f, scene);

		CCDirector::sharedDirector()->replaceScene(transition);
	}
	void replaceSceneWithLayer(CCLayer *layer) {
		CCScene *scene = CCScene::create();
		scene->addChild(layer);

		replaceScene(scene);
	}

	void moveIntoLevel() {
		replaceScene(PlayLayer::scene(loadedLevel, false, false));
	}

	template<typename T>
	std::vector<T *> findInstancesOfObj(cocos2d::CCNode *node) {
		CCArray *children = node->getChildren();

		std::vector<T *> objs = {};

		for (int i = 0; i < children->count(); i++) {
			cocos2d::CCObject *_obj = children->objectAtIndex(i);

			T *dyn = typeinfo_cast<T *>(_obj);

			if (dyn != nullptr) {
				objs.push_back(dyn);
			}
		}

		return objs;
	}
};

class $modify(NPlayLayer, PlayLayer) {
	struct Fields {
		float player_x_old;
		float player_x_new;
		float player_x_delta;

		bool levelStarted = false;
		bool runningAnimation = false;
	};

	bool isPlayerDead() {
		return m_player1->m_isDead || m_player2->m_isDead;
	}

	void saveKilledPlayer() {
		if (NGlobal::destroyID != 0) return;

		GameManager *manager = GameManager::get();
		int frame = manager->getPlayerFrame();

		NGlobal::destroyID = frame;

		log::info("player frame: {}", frame);
		NGlobal::deadIcons.push_back(NGlobal::destroyID);
		NGlobal::save();
	}

	void updateVisibility(float delta) {
		PlayLayer::updateVisibility(delta);

		m_fields->player_x_old = m_fields->player_x_new;
		m_fields->player_x_new = m_player1->getPositionX();

		m_fields->player_x_delta = m_fields->player_x_new - m_fields->player_x_old;

		if (m_fields->player_x_delta != 0.f) m_fields->levelStarted = true;

		if (isPlayerDead()) {
			saveKilledPlayer();
		}
	}
	
	void setupIconLoss(float delta) {
		NGlobal::garageDestroyIcon = true;

		auto scene = GJGarageLayer::scene();
		auto transition = CCTransitionFade::create(1.f, scene);

		CCDirector::sharedDirector()->replaceScene(transition);
	}

	void beginIconLoss() {
		log::info("beginIconLoss()");

		const auto& winSize = CCDirector::sharedDirector()->getWinSize();

		auto base = CCSprite::create("square.png");
		base->setPosition({ 0, 0 });
		base->setScale(500.f);
		base->setColor({0, 0, 0});
		base->setOpacity(0);
		base->runAction(CCFadeTo::create(1.f, 255));

		addChild(base, 9999);
		
		scheduleOnce(schedule_selector(NPlayLayer::setupIconLoss), 1.f);

		NGlobal::loadedLevel = NGlobal::copyLevel(NGlobal::loadedLevelL);
		// NGlobal::deadIcons.clear();
		// for (int i = 0; i < 16; i++) {
		// 	NGlobal::deadIcons.push_back(i);
		// }
	}

	void resetLevel() {
		// bool softEnable = Mod::get()->getSettingValue<bool>("soft-enable");

		if (isPlayerDead()) {
			beginIconLoss();
		} else {
			PlayLayer::resetLevel();
		}
	}

	bool init(GJGameLevel *level, bool a, bool b) {
		if (!PlayLayer::init(level, a, b)) return false;

		NGlobal::garageDestroyIcon = false;
		NGlobal::destroyID = 0;
		NGlobal::loadedLevelL = level;

		return true;
	}

	bool canPauseGame() {
		if (m_fields->runningAnimation == true) return false;
		
		return PlayLayer::canPauseGame();
	}

	void showRetryLayer() {
		if (NGlobal::destroyID != 0) return;

		PlayLayer::canPauseGame();
	}

	void fullReset() {
		log::info("FullReset()");
		PlayLayer::fullReset();
	}
};

#include <cmath>

class XLayer : public CCLayer {
private:
	CCSprite *_x = nullptr;

	float _duration = 0.4f;
	float _waveScale = 20.f;
	float _particleScale = 0.5f;
	float _baseScale = 2.f;

	bool _noRandom = false;
public:
	CREATE_FUNC(XLayer);

	CCMenuItemSpriteExtra *_btn = nullptr;

	void alignNode(CCNode *nd) {
		auto csz = _btn->getContentSize();

		nd->setPosition({csz.width / 2.f, csz.height / 2.f});
	}

	void runAnimation(float delta) {
		_x->setScale(_baseScale);
		
		float dur = _duration;
		
    	_x->runAction(cocos2d::CCEaseBounceOut::create(cocos2d::CCScaleTo::create(dur, 1.0)));
		_x->runAction(CCFadeTo::create(dur, 255));

		grayOutBox();

		CCCircleWave *wave = CCCircleWave::create(10.f, _waveScale, dur, false, true);	
		wave->m_color = ccRED;
		alignNode(wave);

		addChild(wave, 1);

		auto particle = CCParticleSystemQuad::create("explodeEffect.plist", false);
		particle->setScale(_particleScale);
#ifdef _WIN32
		particle->setOpacity(128);
#endif
		alignNode(particle);

		addChild(particle);
	}

	void setupAnimation(float timeOffset) {
		float r = static_cast<float>(rand()) / static_cast<float>(RAND_MAX) * 0.5f + timeOffset;

		if (_noRandom) {
			r = timeOffset;
		}

		scheduleOnce(schedule_selector(XLayer::runAnimation), r);
	}

	bool init() {
		CCSprite *spr = CCSprite::createWithSpriteFrameName("GJ_deleteIcon_001.png");

		spr->setScale(1.f);

		addChild(spr, 2);

		_x = spr;
		_x->setOpacity(0);

		return true;
	};

	void fixPosition() {
		alignNode(_x);
	}

	void grayOutBox() {
		auto base = cocos2d::CCSprite::create("square.png");

		base->setPosition({ 0, 0 });
		base->setScale(3.f);
		base->setColor({0, 0, 0});
		base->setOpacity(0);
		base->runAction(cocos2d::CCFadeTo::create(0.5f, 125));
		
		alignNode(base);
		
		addChild(base, 0);
	}
	
	void setDuration(float dur) {
		_duration = dur;
	}
	void setWaveScale(float scale) {
		_waveScale = scale;
	}
	void setParticleScale(float scale) {
		_particleScale = scale;
	}
	void setBaseScale(float scale) {
		_baseScale = scale;
	}

	void disableRandomTime() {
		_noRandom = true;
	}
};

class DeadScreenLayer : public CCLayer {
private:
	CCLabelBMFont *_label;
public:
	CREATE_FUNC(DeadScreenLayer);

	bool init() {
		if (!CCLayer::init()) return false;

		std::string player_name;
		if (NGlobal::iconNames.count(NGlobal::destroyID)) {
			player_name = NGlobal::iconNames[NGlobal::destroyID];
		} else {
			player_name = "Unknown player";
		}

		std::string label = fmt::format("{} died!", player_name);

		_label = CCLabelBMFont::create(label.c_str(), "goldFont.fnt");
		auto label_csz = _label->getContentSize();

		auto winSize = cocos2d::CCDirector::sharedDirector()->getWinSize();

		CCPoint pos = {winSize.width / 2, winSize.height - label_csz.height - 50};

		_label->setPosition(pos);
		pos.y -= 50;
		_label->runAction(cocos2d::CCEaseExponentialOut::create(cocos2d::CCMoveTo::create(0.5f, pos)));
		// _label->setAnchorPoint({0.f, 0.5f});
		_label->setOpacity(0);
		_label->runAction(cocos2d::CCFadeTo::create(0.5f, 255));

		addChild(_label);

		return true;
	}
};

class IconNamingPopup : public FLAlertLayer {
private:
	int _iconID = 0;
public:
	void onExitButton(CCObject *sender) {
		removeMeAndCleanup();
	}

    void update(float delta) override {
		FLAlertLayer::update(delta);

		return;
	}

    static IconNamingPopup* create(int iconID) {
		IconNamingPopup* pRet = new IconNamingPopup(); 
		if (pRet && pRet->init(iconID)) { 
			pRet->autorelease();
			return pRet;
		} else {
			delete pRet;
			pRet = 0;
			return 0; 
		} 
	}
    bool init(int iconID) {
		if (!FLAlertLayer::init(0)) return false;

		_iconID = iconID;

		CCLayer *objectSelector = CCLayer::create();
		CCLayer *scale9layer = CCLayer::create();

		CCScale9Sprite *spr1 = CCScale9Sprite::create("GJ_square01.png");
		auto winsize = CCDirector::sharedDirector()->getWinSize();

		spr1->setContentSize({300, 125});

		scale9layer->addChild(spr1);
		objectSelector->addChild(scale9layer, 0);

		scale9layer->setPosition({winsize.width / 2, winsize.height / 2});

		auto bmf = CCLabelBMFont::create("Set Icon Name", "bigFont.fnt");
		bmf->setScale(0.8f);
		bmf->setPositionX(winsize.width / 2);
		bmf->setPositionY(winsize.height / 2 + spr1->getContentSize().height / 2 - 30);
				
		objectSelector->addChild(bmf, 1);

		auto exitBtn = CCSprite::createWithSpriteFrameName("GJ_closeBtn_001.png");
		auto btn3 = CCMenuItemSpriteExtra::create(
			exitBtn, this, menu_selector(IconNamingPopup::onExitButton)
		);

		CCMenu *men2 = CCMenu::create();
    
		men2->setPosition({
			winsize.width / 2 - spr1->getContentSize().width / 2,
			winsize.height / 2 + spr1->getContentSize().height / 2
		});
		men2->addChild(btn3);

		objectSelector->addChild(men2, 2);
		
		TextInput *in = TextInput::create(200, "Enter icon name...", "chatFont.fnt");
		in->setPosition(winsize.width / 2, winsize.height / 2);
		in->setAnchorPoint({0.5f, 0.5f});
		in->setCallback([this](const std::string &value) {
			NGlobal::iconNames[this->_iconID] = value;

			NGlobal::save();
		});

		if (NGlobal::iconNames.count(iconID)) {
			in->setString(NGlobal::iconNames[iconID]);
		}

		objectSelector->addChild(in, 2);

		m_mainLayer->addChild(objectSelector);

		auto base = CCSprite::create("square.png");
		base->setPosition({ 0, 0 });
		base->setScale(500.f);
		base->setColor({0, 0, 0});
		base->setOpacity(0);
		base->runAction(CCFadeTo::create(0.3f, 125));

		this->addChild(base, -1);

    	show();

		return true;
	}

	void registerWithTouchDispatcher() override {
		CCTouchDispatcher *dispatcher = cocos2d::CCDirector::sharedDirector()->getTouchDispatcher();

    	dispatcher->addTargetedDelegate(this, -502, true);
	}
};

#include <unordered_map>

class $modify(NGJGarageLayer, GJGarageLayer) {
	struct Fields {
		bool _processingAnimation = false;
		float _currentSoundVol = 1.f;
		CCMenuItemSpriteExtra *_targetedIcon;
		CCMenuItemSpriteExtra *_selectedIcon;
		int _targetedIconID = 0;
		int _selectedIconID = 0;
		int _currentIconID = 0;
		bool _clickedOnLockedIcon = false;
		IconType _pageType;
	};

	// std::unordered_map<CCMenuItemSpriteExtra *, bool> _iconTable;

	CCPoint getGlobalPositionForNode(CCNode *node) {
		CCPoint p = {0.f, 0.f};

		p.x += node->getPosition().x;
		p.y += node->getPosition().y;

		CCNode *main_parent = this;
		CCNode *parent = node->getParent();

		int iter = 0;

		while (parent != nullptr) {
			p.x += parent->getPosition().x;
			p.y += parent->getPosition().y;

			parent = parent->getParent();

			iter++;
		}
	
		log::info("iter={}, parent=nullptr, pos={}", iter, p);

		return p;
	}

	void moveCameraToNode(CCNode *node) {
		auto csz = this->getContentSize();
		auto pos = node->getPosition();
		auto ndcsz = node->getContentSize();
		auto gpos = getGlobalPositionForNode(node);

		CCPoint pointInSpace = {
			-(pos.x) * 2.f,
			gpos.y + (ndcsz.height / 2)
		};

		// pointInSpace.x += csz.width / 2;
		// pointInSpace.y += csz.height / 2;

		// float _x = -(pos.x) * 2.f;
		// float _y = (csz.height - (gpos.y / 2)) / 2.f;

		// if (_y > (csz.height / 2.f)) {
		// 	_y = csz.height / 2.f;
		// }

		runAction(cocos2d::CCEaseInOut::create(cocos2d::CCScaleTo::create(1.f, 2.f), 2.f));
		runAction(cocos2d::CCEaseInOut::create(cocos2d::CCMoveTo::create(1.f, pointInSpace), 2.f));
	}

	void playDEffect1(float delta) {
		FMODAudioEngine *engine = FMODAudioEngine::sharedEngine();
		engine->playEffect("explode_11.ogg", 1.f, 0.5f, m_fields->_currentSoundVol);
		
		log::info("vol={}", m_fields->_currentSoundVol);

		m_fields->_currentSoundVol /= 1.8f;
	}

	void beginGarageRestart(float delta) {
		auto scene = GJGarageLayer::scene();
		auto transition = CCTransitionFade::create(0.5f, scene);

		CCDirector::sharedDirector()->replaceScene(transition);
	}

	void setupGarageRestart(float delta) {
		auto base = cocos2d::CCSprite::create("square.png");

		base->setPosition({ 0, 0 });
		base->setScale(500.f);
		base->setColor({0, 0, 0});
		base->setOpacity(0);
		base->runAction(cocos2d::CCFadeTo::create(1.f, 255));

		addChild(base, 9999);

		scheduleOnce(schedule_selector(NGJGarageLayer::beginGarageRestart), 1.3f);

		NGlobal::destroyID = 0;
	}

	void addDeadScreen(float delta) {
		CCScene *scene = CCScene::get();
		auto scr = DeadScreenLayer::create();

		scene->addChild(scr);

		scheduleOnce(schedule_selector(NGJGarageLayer::setupGarageRestart), 2.f);
	}

	void setupDestroyAnimationGeneric(float delta) {
		auto winSize = cocos2d::CCDirector::sharedDirector()->getWinSize();
    	auto base = cocos2d::CCSprite::create("square.png");

		base->setPosition({ 0, 0 });
		base->setScale(500.f);
		base->setColor({0, 0, 0});
		base->setOpacity(0);
		base->runAction(cocos2d::CCFadeTo::create(0.5f, 215));

		moveCameraToNode(m_fields->_targetedIcon);

		auto pos = getGlobalPositionForNode(m_fields->_targetedIcon);
		m_fields->_targetedIcon->setPosition(pos);

		auto x = XLayer::create();
		x->setDuration(1.f);
		x->setWaveScale(50.f);
		x->setParticleScale(0.8f);	
		x->_btn = m_fields->_targetedIcon;
		x->fixPosition();
		x->setBaseScale(5.f);
		x->disableRandomTime();
		x->setupAnimation(1.f);

		// for (int i = 0; i < 6; i++) {
		// 	float time = 0.2f * (float)i;

		// 	scheduleOnce(schedule_selector(NGJGarageLayer::playDEffect1), 1.f + time);
		// }
		schedule(schedule_selector(NGJGarageLayer::playDEffect1), 0.2f, 16, 1.f);
		scheduleOnce(schedule_selector(NGJGarageLayer::addDeadScreen), 1.f);

		m_fields->_targetedIcon->addChild(x);

		// m_fields->_targetedIcon->setParent(this);
		// m_fields->_targetedIcon->setZOrder(1000);

		addChild(base, 999);
		addChild(m_fields->_targetedIcon, 1000);

		log::info("pos: {}", pos);

		NGlobal::garageBeginIconSelect = true;
	}

	void setupDestroyAnimation(bool final) {
		float time = 2.f;

		scheduleOnce(schedule_selector(NGJGarageLayer::setupDestroyAnimationGeneric), time);
	}

	void setupTestAnimation() {
		// NGlobal::garageDestroyIcon = true;
		// NGlobal::destroyID = 16;
		// NGlobal::deadIcons.clear();
		// for (int i = 0; i < 16; i++) {
		// 	NGlobal::deadIcons.push_back(i);
		// }
	}

	int getPlayerFrame() {
		GameManager *manager = GameManager::get();
		int frame = manager->getPlayerFrame();

		return frame;
	}

	void setupVariables() {
		auto menu = findIconsPage();
		int frame = getPlayerFrame();

		m_fields->_processingAnimation = false;
		m_fields->_targetedIcon = (CCMenuItemSpriteExtra *)menu->getChildByTag(NGlobal::destroyID);
		m_fields->_targetedIconID = NGlobal::destroyID;
		if (m_fields->_targetedIconID == 0) {
			m_fields->_targetedIconID = frame;
		}
		m_fields->_currentIconID = m_currentIcon->getTag();

		m_fields->_selectedIconID = frame;
		m_fields->_currentIconID = frame;
	}

	void setupIconSelectNotGDOne() {
		int icons_per_page = 12 * 3;

		float _page = NGlobal::destroyID / (float)icons_per_page;
		int page = std::ceil(_page) - 1;

		setupPage(page, IconType::Cube);

		auto menu = findIconsPage();

		setupVariables();
	}

	void resetNuzlockePopup(CCObject *sender) {
		if (m_fields->_processingAnimation) {
			FLAlertLayer::create("Nuzlocke", "<cy>I'll reset you.</c>", "Noooo")->show();

			return;
		}

		NGlobal::giveUp->setupInsideGarage();
		NGlobal::printResetPopup();
	}

	bool init() {
		// setupTestAnimation();
		NGlobal::currentGarage = this;

		bool val = NGlobal::garageDestroyIcon;
		bool val2 = NGlobal::garageBeginIconSelect;

		const auto& winSize = CCDirector::sharedDirector()->getWinSize();

		auto bs = ButtonSprite::create("Reset Nuzlocke");
		auto mn = CCMenu::create();
		auto resetbtn = CCMenuItemSpriteExtra::create(
			bs,
			this,
			menu_selector(NGJGarageLayer::resetNuzlockePopup)
		);

		bs->setScale(0.5f);

		auto csz = bs->getContentSize();
		csz.width *= 0.5f;
		csz.height *= 0.5f;

		// resetbtn->setContentSize(csz);

		mn->addChild(resetbtn);
		mn->setPositionY(winSize.height - (csz.height) - 2.5f);

		mn->setID("reset-menu"_spr);
		resetbtn->setID("reset-btn"_spr);

		if (!GJGarageLayer::init()) return false;

		addChild(mn, 10);

		if (val2) {
			setupIconSelectNotGDOne();

			return true;
		}

		if (val) {
			int icons_per_page = 12 * 3;

			float _page = NGlobal::destroyID / (float)icons_per_page;
			int page = std::ceil(_page) - 1;

			setupPage(page, IconType::Cube);

			auto menu = findIconsPage();

			setupVariables();
			m_fields->_processingAnimation = true;

			setupDestroyAnimation(false);

			return true;
		}

		return true;
	}

	CCMenu *findIconsPage() {
#ifdef _WIN32
		CCNode *lbb = m_iconSelection;
		if (!lbb) return nullptr;
#endif
#ifndef _WIN32
		CCNode *lbb = this->getChildByID("icon-selection-bar");
		if (!lbb) return nullptr;
#endif

#define FIND_OBJECT_INSTANCE(obj, var, node) std::vector<obj *> v_##var = NGlobal::findInstancesOfObj<obj>(node); if (v_##var.size() == 0) return nullptr; auto var = v_##var[0]

		FIND_OBJECT_INSTANCE(BoomScrollLayer, obj1, lbb);
		obj1->setID("scroll-layer");

		FIND_OBJECT_INSTANCE(ExtendedLayer, obj2, obj1);
		obj2->setID("extended-layer");

		FIND_OBJECT_INSTANCE(ListButtonPage, obj3, obj2);
		obj3->setID("icons-page");

		FIND_OBJECT_INSTANCE(CCMenu, obj4, obj3);
		obj4->setID("icons-menu");

#undef FIND_OBJECT_INSTANCE

		return obj4;
	}

	void setupPage(int page, IconType type) {
		if (m_fields->_processingAnimation) return;	

		m_fields->_pageType = type;	

		GJGarageLayer::setupPage(page, type);

		auto menu = findIconsPage();

		if (type == IconType::Cube) {
			if (!menu) {
				log::info("unable to find icon menu!\n");

				return;
			}
			
			auto icons = menu->getChildren();

			for (int i = 0; i < icons->count(); i++) {
				auto obj = static_cast<CCMenuItemSpriteExtra *>(icons->objectAtIndex(i));

				if (!obj) continue;

				auto v = NGlobal::deadIcons;

				if(std::find(v.begin(), v.end(), obj->getTag()) != v.end()) {
					if (obj->getTag() != NGlobal::destroyID) {
						float offset = 0.f;

						if (NGlobal::garageDestroyIcon) {
							offset = 1.f;
						}

						auto x = XLayer::create();
						x->_btn = obj;

						x->fixPosition();
						x->setupAnimation(offset);

						obj->addChild(x);
						obj->setEnabled(false);
					}
				}
			}

			NGlobal::garageDestroyIcon = false;
		}
	}

	void onNavigate(CCObject *sender) {
		log::info("a");

		if (m_fields->_processingAnimation) return;

		GJGarageLayer::onNavigate(sender);
	}
	
	// void onShop(CCObject *sender) {
	// 	log::info("b");

	// 	// if (m_fields->_processingAnimation) return;

	// 	GJGarageLayer::onShop(sender);
	// }

	void onSelect(CCObject *sender) {
		log::info("c");

		if (m_fields->_pageType != IconType::Cube) {
			GJGarageLayer::onSelect(sender);

			return;
		}

		if (m_fields->_processingAnimation) return;

		m_fields->_clickedOnLockedIcon = false;

		GJGarageLayer::onSelect(sender);

		if (m_fields->_clickedOnLockedIcon) return;

		int tag = sender->getTag();
		m_fields->_selectedIcon = (CCMenuItemSpriteExtra *)sender;
		m_fields->_selectedIconID = tag;

		IconNamingPopup *popup = IconNamingPopup::create(tag);
	}

	void onSelectTab(CCObject *sender) {
		log::info("d");

		if (m_fields->_processingAnimation) return;

		GJGarageLayer::onSelectTab(sender);
	}

	bool verifyIcon() {
		log::info("do we doing icon anim?");
		if (m_fields->_processingAnimation) return false;

		int frame = getPlayerFrame();
		
		log::info("frame={}", frame);

		auto icon = frame;
		// if (icon == 0) icon = m_fields->_currentIconID;
		// if (icon == 0) icon = frame;

		log::info("icon={}", icon);

		log::info("no. do we selecting icon? if yes then did player change icon?");
		if (NGlobal::garageBeginIconSelect && (m_fields->_targetedIconID == icon)) {
			log::info("he did not. returning error");
			NGlobal::giveUp->setupInsideGarage();
			NGlobal::printNewIconError();

			return false;
		}

		log::info("seems that player did change their icon. or garage is not in select mode. anyway thats good");
		log::info("heres some proofs: icon={}; target={}; garageBeginIconSelect={}", icon, m_fields->_targetedIconID, NGlobal::garageBeginIconSelect);

		NGlobal::garageBeginIconSelect = false;
		
		int tag = icon;

		log::info("do we know about this icon?");
		if (!NGlobal::iconNames.count(tag)) {
			log::info("no. tag={}, no username available, creating popup...", tag);

			IconNamingPopup *popup = IconNamingPopup::create(tag);

			return false;
		}
		log::info("we know. everything is good. keep playing, gd player");

		return true;
	}

	void keyBackClicked() {
		log::info("e");

		bool val2 = NGlobal::garageBeginIconSelect;

		if (verifyIcon()) {
			log::info("verify icon succeded");
			if (!val2) {
				GJGarageLayer::keyBackClicked();
			} else {
				NGlobal::moveIntoLevel();
			}
		}
	}

	void onBack(CCObject *sender) {
		log::info("f");

		bool val2 = NGlobal::garageBeginIconSelect;

		if (verifyIcon()) {
			log::info("verify icon succeded");
			if (!val2) {
				GJGarageLayer::onBack(sender);
			} else {
				NGlobal::moveIntoLevel();;
			}
		}
	}

	void showUnlockPopup(int p0, UnlockType p1) {
		m_fields->_clickedOnLockedIcon = true;

		GJGarageLayer::showUnlockPopup(p0, p1);
	}
};

class $modify(NMenuLayer, MenuLayer) {
	bool askForIconName() {
		log::info("asking for icon name...");

		GameManager *manager = GameManager::get();
		int frame = manager->getPlayerFrame();

		if (!NGlobal::iconNames.count(frame)) {
			log::info("no username available, creating popup...");

			IconNamingPopup *popup = IconNamingPopup::create(frame);

			return false;
		}

		log::info("what if player has been killed?");

		auto v = NGlobal::deadIcons;

		if(std::find(v.begin(), v.end(), frame) != v.end()) {
			log::info("it WAS killed. ok");

			NGlobal::giveUp->setupOutsideGarage();
			NGlobal::printDeadIconError();

			return false;
		}

		log::info("it WAS NOT killed");

		return true;
	}

	void askForIconNameS(float delta) {
		if (!NGlobal::newAccountPopupShown) {
			FLAlertLayer::create("Nuzlocke", "<cg>Nuzlocke Challenge</c> mod recommends you to <cr>unlink</c> your account before playing with this mod.\n<cy>If unlinking please make sure that your savedata is backed up!!</c>", "OK")->show();
		
			NGlobal::newAccountPopupShown = true;
			NGlobal::save();

			return;
		}

		askForIconName();
	}

	bool init() {
		if (!MenuLayer::init()) return false;

		scheduleOnce(schedule_selector(NMenuLayer::askForIconNameS), 0.55f);

		log::info("CCNode size: {}", sizeof(CCNode));

		return true;
	}

	void onCreator(CCObject *sender) {
		if (!askForIconName()) {
			return;
		}

		MenuLayer::onCreator(sender);
	}
	void onPlay(CCObject *sender) {
		if (!askForIconName()) {
			return;
		}

		MenuLayer::onPlay(sender);
	}
	void onMyProfile(CCObject *sender) {
		if (!askForIconName()) {
			return;
		}

		MenuLayer::onMyProfile(sender);
	}
};

$execute {
	NGlobal::setupProtocols();
	NGlobal::recover();
}

class $modify(NPauseLayer, PauseLayer) {
	bool checkPlayer() {
		return NGlobal::destroyID == 0; 
	}

#define PLCALL(func) void func(CCObject *sender) { if (!checkPlayer()) return; PauseLayer::func(sender); }

	PLCALL(onEdit);
	PLCALL(onNormalMode);
	PLCALL(onPracticeMode);
	PLCALL(onRestart);
	PLCALL(onRestartFull);
	PLCALL(onQuit);
	PLCALL(tryQuit);
	
#undef PLCALL
};