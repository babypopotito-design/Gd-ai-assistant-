#include <Geode/Geode.hpp>
#include <Geode/modify/LevelEditorLayer.hpp>
#include <Geode/utils/web.hpp>
#include <vector>

using namespace geode::prelude;

static constexpr char kRelayURL[] = "https://8080-i19ilsqpehq6phaakir54-ce454132.us3.manus.computer/assist";

struct EditorAction {
    std::string operation;
    int uniqueID;
    int objectID;
    float x;
    float y;
    float scale;
    float rotation;
};

class AssistantPopup final : public Popup {
protected:
    TextInput* m_prompt = nullptr;
    CCLabelBMFont* m_status = nullptr;
    CCLabelBMFont* m_preview = nullptr;
    async::TaskHolder<web::WebResponse> m_listener;
    std::vector<EditorAction> m_actions;

    bool init() {
        if (!Popup::init(380.f, 300.f)) return false;
        this->setTitle("GD AI Assistant");

        m_prompt = TextInput::create(290.f, "Tell me what to build...");
        m_prompt->setID("prompt-input");
        m_mainLayer->addChildAtPosition(m_prompt, Anchor::Center, {0.f, 72.f});

        auto menu = CCMenu::create();
        menu->setContentSize({360.f, 44.f});
        menu->setPosition({190.f, 116.f});
        menu->setID("assistant-actions");
        m_mainLayer->addChild(menu);

        auto ask = CCMenuItemSpriteExtra::create(
            ButtonSprite::create("Ask", "goldFont.fnt", "GJ_button_01.png", .75f),
            this, menu_selector(AssistantPopup::onAsk)
        );
        ask->setID("ask-button");
        menu->addChildAtPosition(ask, Anchor::Center, {-65.f, 0.f});

        auto apply = CCMenuItemSpriteExtra::create(
            ButtonSprite::create("Apply", "goldFont.fnt", "GJ_button_01.png", .75f),
            this, menu_selector(AssistantPopup::onApply)
        );
        apply->setID("apply-button");
        menu->addChildAtPosition(apply, Anchor::Center, {65.f, 0.f});

        m_status = CCLabelBMFont::create("Ask the AI for a build plan.", "goldFont.fnt");
        m_status->setScale(.5f);
        m_status->setWidth(310.f);
        m_status->setAlignment(kCCTextAlignmentCenter);
        m_mainLayer->addChildAtPosition(m_status, Anchor::Center, {0.f, 35.f});

        m_preview = CCLabelBMFont::create("No preview yet.", "goldFont.fnt");
        m_preview->setScale(.42f);
        m_preview->setWidth(310.f);
        m_preview->setAlignment(kCCTextAlignmentCenter);
        m_mainLayer->addChildAtPosition(m_preview, Anchor::Center, {0.f, -40.f});
        return true;
    }

    void onAsk(CCObject*) {
        auto prompt = m_prompt->getString();
        if (prompt.empty()) {
            m_status->setString("Type an instruction first.");
            return;
        }
        m_actions.clear();
        m_preview->setString("Waiting for an AI plan...");
        auto body = matjson::Value();
        body["prompt"] = matjson::Value(prompt.c_str());
        std::string context = "Existing objects (uniqueID, objectID, x, y):\n";
        if (auto editor = LevelEditorLayer::get()) {
            auto objects = editor->getAllObjects();
            CCARRAY_FOREACH(objects, node) {
                auto object = typeinfo_cast<GameObject*>(node);
                if (!object) continue;
                context += fmt::format("{}, {}, {:.1f}, {:.1f}\n", object->m_uniqueID, object->m_objectID, object->getPositionX(), object->getPositionY());
                if (context.size() > 12000) break;
            }
        }
        body["context"] = matjson::Value(context.c_str());
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
                self->m_status->setString("The AI returned an invalid plan.");
                return;
            }
            auto root = json.unwrap();
            auto messageValue = root.get("message");
            auto message = messageValue ? messageValue.unwrap().asString().unwrapOr("Plan ready.") : "Plan ready.";
            self->m_status->setString(message.c_str());

            auto actionsValue = root.get("actions");
            if (!actionsValue) {
                self->m_preview->setString("No editor actions proposed.");
                return;
            }
            auto array = actionsValue.unwrap().asArray();
            if (!array) {
                self->m_preview->setString("No editor actions proposed.");
                return;
            }
            for (auto const& value : array.unwrap()) {
                auto type = value.get("object");
                auto operation = value.get("operation");
                auto uniqueID = value.get("uniqueID");
                auto x = value.get("x");
                auto y = value.get("y");
                auto scale = value.get("scale");
                auto rotation = value.get("rotation");
                if (!type || !operation || !uniqueID || !x || !y || !scale || !rotation) continue;
                auto typeName = type.unwrap().asString().unwrapOr("");
                auto operationName = operation.unwrap().asString().unwrapOr("place");
                int objectID = typeName == "spike" ? 8 : 1;
                self->m_actions.push_back({
                    operationName,
                    static_cast<int>(uniqueID.unwrap().asInt().unwrapOr(0)),
                    objectID,
                    static_cast<float>(x.unwrap().asDouble().unwrapOr(0.0)),
                    static_cast<float>(y.unwrap().asDouble().unwrapOr(150.0)),
                    static_cast<float>(scale.unwrap().asDouble().unwrapOr(1.0)),
                    static_cast<float>(rotation.unwrap().asDouble().unwrapOr(0.0))
                });
            }
            self->m_preview->setString(fmt::format("Preview: {} objects. Tap Apply to place them.", self->m_actions.size()).c_str());
        });
    }

    void onApply(CCObject*) {
        auto editor = LevelEditorLayer::get();
        if (!editor || m_actions.empty()) {
            m_status->setString("Ask for a plan first.");
            return;
        }
        for (auto const& action : m_actions) {
            if (action.operation == "delete") {
                if (auto object = editor->findGameObject(action.uniqueID)) {
                    editor->removeObject(object, false);
                }
                continue;
            }
            if (action.operation == "move") {
                if (auto object = editor->findGameObject(action.uniqueID)) {
                    object->setPosition({action.x, action.y});
                    object->setRotation(action.rotation);
                    object->setScale(action.scale);
                    editor->objectMoved(object);
                }
                continue;
            }
            auto object = editor->createObject(action.objectID, {action.x, action.y}, false);
            if (object) {
                object->setScale(action.scale);
                object->setRotation(action.rotation);
            }
        }
        m_status->setString("Applied the approved preview.");
        m_actions.clear();
        m_preview->setString("No pending actions.");
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
        auto button = CCMenuItemSpriteExtra::create(icon, this, menu_selector(AIAssistantEditorLayer::onOpenAssistant));
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
