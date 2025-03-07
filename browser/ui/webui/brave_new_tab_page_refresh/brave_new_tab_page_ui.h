// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_WEBUI_BRAVE_NEW_TAB_PAGE_REFRESH_BRAVE_NEW_TAB_PAGE_UI_H_
#define BRAVE_BROWSER_UI_WEBUI_BRAVE_NEW_TAB_PAGE_REFRESH_BRAVE_NEW_TAB_PAGE_UI_H_

#include <memory>

#include "brave/browser/ui/webui/brave_new_tab_page_refresh/brave_new_tab_page.mojom.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "ui/webui/mojo_web_ui_controller.h"

// The Web UI controller for the Brave new tab page.
class BraveNewTabPageUI : public ui::MojoWebUIController {
 public:
  explicit BraveNewTabPageUI(content::WebUI* web_ui);
  ~BraveNewTabPageUI() override;

  void BindInterface(
      mojo::PendingReceiver<
          brave_new_tab_page_refresh::mojom::NewTabPageHandler> receiver);

 private:
  std::unique_ptr<brave_new_tab_page_refresh::mojom::NewTabPageHandler>
      page_handler_;

  WEB_UI_CONTROLLER_TYPE_DECL();
};

#endif  // BRAVE_BROWSER_UI_WEBUI_BRAVE_NEW_TAB_PAGE_REFRESH_BRAVE_NEW_TAB_PAGE_UI_H_
