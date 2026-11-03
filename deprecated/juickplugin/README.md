# Juick plugin (deprecated)

This archived plugin was retired from normal builds when Psi removed the WebKit/WebEngine chat backend and `WebkitAccessingHost` plugin interface.

It still contains useful Juick stanza parsing, XHTML-IM formatting, links, and image retrieval logic. The sources are preserved for a future native `MamChatWidget` / `MessageView` integration. They are **not compatible with the current Psi plugin API** and are deliberately excluded from `BUILD_PLUGINS=ALL`.

To revive the plugin, replace its old `WebkitAccessor` dependency and adapt message presentation to Psi's native chat rather than modifying the hidden `ChatView`.
