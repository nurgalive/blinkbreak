#include "platform_interface.hpp"

namespace blinkbreak::platform
{

class NullTrayIcon : public ITrayIcon
{
public:
  bool Show() override { return true; }
  void Hide() override {}
  void SetTooltip(const std::string& /*tooltip*/) override {}
  void SetMenu(const std::vector<MenuItem>& /*items*/) override {}
  void SetOnClick(std::function<void()> callback) override { on_click_ = std::move(callback); }
  void SetOnDoubleClick(std::function<void()> callback) override
  {
    on_double_click_ = std::move(callback);
  }
  void SetHostWindow(std::uintptr_t /*native_window_handle*/) override {}
  void SetIcon(int /*icon_id*/) override {}

private:
  std::function<void()> on_click_;
  std::function<void()> on_double_click_;
};

std::unique_ptr<ITrayIcon> CreateNullTrayIcon()
{
  return std::make_unique<NullTrayIcon>();
}

}  // namespace blinkbreak::platform
