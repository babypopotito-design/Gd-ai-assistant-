#include <Geode/Geode.hpp>
#include <Geode/modify/LevelEditorLayer.hpp>
#include <Geode/utils/web.hpp>

using namespace geode::prelude;

static constexpr char kRelayURL[] = "https://8080-i19ilsqpehq6phaakir54-ce454132.us3.manus.computer/assist";

class AssistantPopup final : public Popup {
protected:
    TextInput* m_prompt = nullptr;
    CCLabelBMFont* m_status = nullptr;
    async::TaskHolder<web::WebResponse> m_listener;

    bool init() {
        if (!Popup::init(360.f, 220.f)) return false;
        this->setTitle("GD AI Assistant");

        m_prompt = TextInput::create(270.f, "Tell me what to build...");
        m_prompt->setID("prompt-input");
        m_mainLayer->addChildAtPosition(m_prompt, Anchor::Center, {0.f, 28.f});

        auto menu = CCMenu::create();
        menu->setContentSize({360.f, 40.f});
        menu->setPosition({180.f, 82.f});
        menu->setID("assistant-actions");
        m_mainLayer->addChild(menu);

        auto send = CCMenuItemSpriteExtra::create(
            ButtonSprite::create("Ask", "goldFont.fnt", "GJ_button_01.png", .8f),
            this,
            menu_selector(AssistantPopup::onAsk)
        );
        send->setID("ask-button");
        menu->addChildAtPosition(send, Anchor::Center);

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

        m_status->setString("Thinking...");
        auto body = matjson::Value();
        body["prompt"] = matjson::Value(prompt.c_str());
        auto req = web::WebRequest();
        req.bodyJSON(body);
        req.header("Content-Type", "application/json");
        req.timeout(std::chrono::seconds(45));
        auto self = Ref(this);
        m_listener.spawn(req.post(kRelayURL), [self](web::WebResponse res) {
            if (!res.ok()) {
                self->m_status->setString("The AI relay could not be reached.");
                return;
            }
            auto json = res.json();
            if (!json) {
                self->m_status->setString("The AI returned an invalid response.");
                return;
            }
            auto replyValue = json.unwrap().get("reply");
            if (!replyValue) {
                self->m_status->setString("The AI returned no text.");
                return;
            }
            auto reply = replyValue.unwrap().asString().unwrapOr("");
            self->m_status->setString(reply.empty() ? "The AI returned no text." : reply);
        });
    }

public:
    static AssistantPopup* create() {
        auto ret = new AssistantPopup();
        if (ret && ret->init()) {
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
