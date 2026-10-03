#include <Geode/Geode.hpp>
#include <Geode/modify/LevelEditorLayer.hpp>

using namespace geode::prelude;

class AssistantPopup final : public Popup<> {
protected:
    TextInput* m_prompt = nullptr;
    CCLabelBMFont* m_status = nullptr;

    bool setup() override {
        this->setTitle("GD AI Assistant");

        m_prompt = TextInput::create(270.f, "Tell me what to build...");
        m_prompt->setID("prompt-input");
        m_mainLayer->addChildAtPosition(m_prompt, Anchor::Center, {0.f, 28.f});

        auto send = CCMenuItemSpriteExtra::create(
            ButtonSprite::create("Ask", "goldFont.fnt", "GJ_button_01.png", .8f),
            this,
            menu_selector(AssistantPopup::onAsk)
        );
        send->setID("ask-button");
        m_buttonMenu->addChildAtPosition(send, Anchor::Center, {0.f, -18.f});

        m_status = CCLabelBMFont::create(
            "Preview mode: connect an endpoint in mod settings.",
            "goldFont.fnt"
        );
        m_status->setScale(.45f);
        m_status->setWidth(270.f);
        m_status->setAlignment(kCCTextAlignmentCenter);
        m_mainLayer->addChildAtPosition(m_status, Anchor::Center, {0.f, -62.f});
        return true;
    }

    void onAsk(CCObject*) {
        auto prompt = m_prompt->getString();
        if (prompt.empty()) {
            m_status->setString("Type an instruction first.");
            return;
        }

        // Safe first milestone: prove the editor UI flow without embedding
        // an API key in the mod. Network integration will call a user-owned
        // HTTPS relay configured through the mod setting.
        m_status->setString("Instruction received. AI relay is not connected yet.");
        log::info("GD AI Assistant prompt: {}", prompt);
    }

public:
    static AssistantPopup* create() {
        auto ret = new AssistantPopup();
        if (ret && ret->initAnchored(360.f, 220.f)) {
            ret->autorelease();
            return ret;
        }
        CC_SAFE_DELETE(ret);
        return nullptr;
    }
};

class $modify(AIAssistantEditorLayer, LevelEditorLayer) {
    bool init(GJGameLevel* level, bool unk) {
        if (!LevelEditorLayer::init(level, unk)) return false;

        auto menu = CCMenu::create();
        menu->setID("gd-ai-assistant-menu");
        menu->setPosition({0.f, 0.f});
        menu->setZOrder(100);

        auto icon = CCSprite::createWithSpriteFrameName("GJ_chatBtn_001.png");
        if (!icon) icon = CCSprite::createWithSpriteFrameName("GJ_infoIcon_001.png");
        icon->setScale(.75f);

        auto button = CCMenuItemSpriteExtra::create(
            icon,
            this,
            menu_selector(AIAssistantEditorLayer::onOpenAssistant)
        );
        button->setID("open-assistant-button");
        menu->addChild(button);
        menu->setContentSize(button->getContentSize());
        menu->setAnchorPoint({0.f, 0.f});
        menu->setPosition({15.f, 90.f});
        this->addChild(menu);
        return true;
    }

    void onOpenAssistant(CCObject*) {
        AssistantPopup::create()->show();
    }
};
