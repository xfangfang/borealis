#include <borealis/core/box.hpp>
#include <borealis/views/label.hpp>
#include <borealis/core/bind.hpp>

namespace brls
{
class EditTextDialog : public Box
{
  public:
    EditTextDialog();
    void open();
    void setText(const std::string& value);
    void setHeaderText(const std::string& value);
    void setHintText(const std::string& value);
    void setCountText(const std::string& value);
    bool isTranslucent() override;
    void onLayout() override;
    Event<Point>* getLayoutEvent();
    Event<>* getCancelEvent();
    Event<>* getSubmitEvent();
    void updateUI();
  private:
    std::string content;
    std::string hint;
    Event<Point> layoutEvent;
    Event<> cancelEvent, summitEvent;
    bool init = false;

    BRLS_BIND(brls::Label, header, "brls/dialog/header");
    BRLS_BIND(brls::Label, label, "brls/dialog/label");
    BRLS_BIND(brls::Label, count, "brls/dialog/count");
    BRLS_BIND(brls::Box, container, "brls/container");
};
}