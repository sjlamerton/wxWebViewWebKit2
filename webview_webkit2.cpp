/////////////////////////////////////////////////////////////////////////////
// Name:        src/gtk/webview_webkit2.cpp
// Purpose:     GTK WebKit backend for web view component
// Author:      Marianne Gagnon, Robert Roebling, Steven Lamerton
// Id:          $Id$
// Copyright:   (c) 2010 Marianne Gagnon, 1998 Robert Roebling
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

// For compilers that support precompilation, includes "wx.h".
#include "wx/wxprec.h"

#if wxUSE_WEBVIEW

#include "webview_webkit2.h"
#include <wx/stockitem.h>
#include <wx/gtk/control.h>
#include <wx/filesys.h>
#include <wx/base64.h>
#include <wx/log.h>
#include <webkit2/webkit2.h>

// ----------------------------------------------------------------------------
// GTK callbacks
// ----------------------------------------------------------------------------

extern "C"
{

static void
wxgtk_webview_webkit_load_changed(WebKitWebView *web_view,
                                  WebKitLoadEvent load_event,
                                  wxWebViewWebKit2 *ctrl)
{
    wxString url = ctrl->GetCurrentURL();

    if (load_event == WEBKIT_LOAD_FINISHED)
    {
        wxWebViewEvent event(wxEVT_WEBVIEW_LOADED, ctrl->GetId(), url, "");

        if (ctrl && ctrl->GetEventHandler())
            ctrl->GetEventHandler()->ProcessEvent(event);
    }
    else if (load_event ==  WEBKIT_LOAD_COMMITTED)
    {
        wxWebViewEvent event(wxEVT_WEBVIEW_NAVIGATED, ctrl->GetId(), url, "");

        if (ctrl && ctrl->GetEventHandler())
            ctrl->GetEventHandler()->ProcessEvent(event);
    }
}
/*
static gboolean
wxgtk_webview_webkit_navigation(WebKitWebView *,
                                WebKitWebFrame *frame,
                                WebKitNetworkRequest *request,
                                WebKitWebNavigationAction *,
                                WebKitWebPolicyDecision *policy_decision,
                                wxWebViewWebKit2 *webKitCtrl)
{
    if(webKitCtrl->m_guard)
    {
        webKitCtrl->m_guard = false;
        //We set this to make sure that we don't try to load the page again from
        //the resource request callback
        webKitCtrl->m_vfsurl = webkit_network_request_get_uri(request);
        webkit_web_policy_decision_use(policy_decision);
        return FALSE;
    }

    webKitCtrl->m_busy = true;

    const gchar* uri = webkit_network_request_get_uri(request);

    wxString target = webkit_web_frame_get_name (frame);
    wxWebViewEvent event(wxEVT_WEBVIEW_NAVIGATING,
                         webKitCtrl->GetId(),
                         wxString( uri, wxConvUTF8 ),
                         target);

    if (webKitCtrl && webKitCtrl->GetEventHandler())
        webKitCtrl->GetEventHandler()->ProcessEvent(event);

    if (!event.IsAllowed())
    {
        webKitCtrl->m_busy = false;
        webkit_web_policy_decision_ignore(policy_decision);
        return TRUE;
    }
    else
    {
        wxString wxuri = uri;
        wxSharedPtr<wxWebViewHandler> handler;
        wxVector<wxSharedPtr<wxWebViewHandler> > hanlders = webKitCtrl->GetHandlers();
        //We are not vetoed so see if we match one of the additional handlers
        for(wxVector<wxSharedPtr<wxWebViewHandler> >::iterator it = hanlders.begin();
            it != hanlders.end(); ++it)
        {
            if(wxuri.substr(0, (*it)->GetName().length()) == (*it)->GetName())
            {
                handler = (*it);
            }
        }
        //If we found a handler we can then use it to load the file directly
        //ourselves
        if(handler)
        {
            webKitCtrl->m_guard = true;
            wxFSFile* file = handler->GetFile(wxuri);
            if(file)
            {
                webKitCtrl->SetPage(*file->GetStream(), wxuri);
            }
            //We need to throw some sort of error here if file is NULL
            webkit_web_policy_decision_ignore(policy_decision);
            return TRUE;
        }
        return FALSE;
    }
}

static gboolean
wxgtk_webview_webkit_error(WebKitWebView*,
                           WebKitWebFrame*,
                           gchar *uri,
                           gpointer web_error,
                           wxWebViewWebKit2* webKitWindow)
{
    webKitWindow->m_busy = false;
    wxWebViewNavigationError type = wxWEBVIEW_NAV_ERR_OTHER;

    GError* error = (GError*)web_error;
    wxString description(error->message, wxConvUTF8);

    if (strcmp(g_quark_to_string(error->domain), "soup_http_error_quark") == 0)
    {
        switch (error->code)
        {
            case SOUP_STATUS_CANCELLED:
                type = wxWEBVIEW_NAV_ERR_USER_CANCELLED;
                break;

            case SOUP_STATUS_CANT_RESOLVE:
            case SOUP_STATUS_NOT_FOUND:
                type = wxWEBVIEW_NAV_ERR_NOT_FOUND;
                break;

            case SOUP_STATUS_CANT_RESOLVE_PROXY:
            case SOUP_STATUS_CANT_CONNECT:
            case SOUP_STATUS_CANT_CONNECT_PROXY:
            case SOUP_STATUS_SSL_FAILED:
            case SOUP_STATUS_IO_ERROR:
                type = wxWEBVIEW_NAV_ERR_CONNECTION;
                break;

            case SOUP_STATUS_MALFORMED:
            //case SOUP_STATUS_TOO_MANY_REDIRECTS:
                type = wxWEBVIEW_NAV_ERR_REQUEST;
                break;

            //case SOUP_STATUS_NO_CONTENT:
            //case SOUP_STATUS_RESET_CONTENT:

            case SOUP_STATUS_BAD_REQUEST:
                type = wxWEBVIEW_NAV_ERR_REQUEST;
                break;

            case SOUP_STATUS_UNAUTHORIZED:
            case SOUP_STATUS_FORBIDDEN:
                type = wxWEBVIEW_NAV_ERR_AUTH;
                break;

            case SOUP_STATUS_METHOD_NOT_ALLOWED:
            case SOUP_STATUS_NOT_ACCEPTABLE:
                type = wxWEBVIEW_NAV_ERR_SECURITY;
                break;

            case SOUP_STATUS_PROXY_AUTHENTICATION_REQUIRED:
                type = wxWEBVIEW_NAV_ERR_AUTH;
                break;

            case SOUP_STATUS_REQUEST_TIMEOUT:
                type = wxWEBVIEW_NAV_ERR_CONNECTION;
                break;

            //case SOUP_STATUS_PAYMENT_REQUIRED:
            case SOUP_STATUS_REQUEST_ENTITY_TOO_LARGE:
            case SOUP_STATUS_REQUEST_URI_TOO_LONG:
            case SOUP_STATUS_UNSUPPORTED_MEDIA_TYPE:
                type = wxWEBVIEW_NAV_ERR_REQUEST;
                break;

            case SOUP_STATUS_BAD_GATEWAY:
            case SOUP_STATUS_SERVICE_UNAVAILABLE:
            case SOUP_STATUS_GATEWAY_TIMEOUT:
                type = wxWEBVIEW_NAV_ERR_CONNECTION;
                break;

            case SOUP_STATUS_HTTP_VERSION_NOT_SUPPORTED:
                type = wxWEBVIEW_NAV_ERR_REQUEST;
                break;
            //case SOUP_STATUS_INSUFFICIENT_STORAGE:
            //case SOUP_STATUS_NOT_EXTENDED:
        }
    }
    else if (strcmp(g_quark_to_string(error->domain),
                    "webkit-network-error-quark") == 0)
    {
        switch (error->code)
        {
            //WEBKIT_NETWORK_ERROR_FAILED:
            //WEBKIT_NETWORK_ERROR_TRANSPORT:

            case WEBKIT_NETWORK_ERROR_UNKNOWN_PROTOCOL:
                type = wxWEBVIEW_NAV_ERR_REQUEST;
                break;

            case WEBKIT_NETWORK_ERROR_CANCELLED:
                type = wxWEBVIEW_NAV_ERR_USER_CANCELLED;
                break;

            case WEBKIT_NETWORK_ERROR_FILE_DOES_NOT_EXIST:
                type = wxWEBVIEW_NAV_ERR_NOT_FOUND;
                break;
        }
    }
    else if (strcmp(g_quark_to_string(error->domain),
                    "webkit-policy-error-quark") == 0)
    {
        switch (error->code)
        {
            //case WEBKIT_POLICY_ERROR_FAILED:
            //case WEBKIT_POLICY_ERROR_CANNOT_SHOW_MIME_TYPE:
            //case WEBKIT_POLICY_ERROR_CANNOT_SHOW_URL:
            //case WEBKIT_POLICY_ERROR_FRAME_LOAD_INTERRUPTED_BY_POLICY_CHANGE:
            case WEBKIT_POLICY_ERROR_CANNOT_USE_RESTRICTED_PORT:
                type = wxWEBVIEW_NAV_ERR_SECURITY;
                break;
        }
    }

    wxWebViewEvent event(wxEVT_WEBVIEW_ERROR,
                         webKitWindow->GetId(),
                         uri, "");
    event.SetString(description);
    event.SetInt(type);

    if (webKitWindow && webKitWindow->GetEventHandler())
    {
        webKitWindow->GetEventHandler()->ProcessEvent(event);
    }

    return FALSE;
}

static gboolean
wxgtk_webview_webkit_new_window(WebKitWebView*,
                                WebKitWebFrame *frame,
                                WebKitNetworkRequest *request,
                                WebKitWebNavigationAction*,
                                WebKitWebPolicyDecision *policy_decision,
                                wxWebViewWebKit2 *webKitCtrl)
{
    const gchar* uri = webkit_network_request_get_uri(request);

    wxString target = webkit_web_frame_get_name (frame);
    wxWebViewEvent event(wxEVT_WEBVIEW_NEWWINDOW,
                                       webKitCtrl->GetId(),
                                       wxString( uri, wxConvUTF8 ),
                                       target);

    if (webKitCtrl && webKitCtrl->GetEventHandler())
        webKitCtrl->GetEventHandler()->ProcessEvent(event);

    //We always want the user to handle this themselves
    webkit_web_policy_decision_ignore(policy_decision);
    return TRUE;
}
*/
static void
wxgtk_webview_webkit_title_changed(WebKitWebView*,
                                   GParamSpec *pspec,
                                   wxWebViewWebKit2 *ctrl)
{
    wxWebViewEvent event(wxEVT_WEBVIEW_TITLE_CHANGED, ctrl->GetId(),
                         ctrl->GetCurrentURL(), "");
    event.SetString(ctrl->GetCurrentTitle());

    if (ctrl && ctrl->GetEventHandler())
        ctrl->GetEventHandler()->ProcessEvent(event);

}
/*
static void
wxgtk_webview_webkit_resource_req(WebKitWebView *,
                                  WebKitWebFrame *,
                                  WebKitWebResource *,
                                  WebKitNetworkRequest *request,
                                  WebKitNetworkResponse *,
                                  wxWebViewWebKit2 *webKitCtrl)
{
    wxString uri = webkit_network_request_get_uri(request);

    wxSharedPtr<wxWebViewHandler> handler;
    wxVector<wxSharedPtr<wxWebViewHandler> > hanlders = webKitCtrl->GetHandlers();

    //We are not vetoed so see if we match one of the additional handlers
    for(wxVector<wxSharedPtr<wxWebViewHandler> >::iterator it = hanlders.begin();
        it != hanlders.end(); ++it)
    {
        if(uri.substr(0, (*it)->GetName().length()) == (*it)->GetName())
        {
            handler = (*it);
        }
    }
    //If we found a handler we can then use it to load the file directly
    //ourselves
    if(handler)
    {
        //If it is requsting the page itself then return as we have already
        //loaded it from the archive
        if(webKitCtrl->m_vfsurl == uri)
            return;

        wxFSFile* file = handler->GetFile(uri);
        if(file)
        {
            //We load the data into a data url to save it being written out again
            size_t size = file->GetStream()->GetLength();
            char *buffer = new char[size];
            file->GetStream()->Read(buffer, size);
            wxString data = wxBase64Encode(buffer, size);
            delete[] buffer;
            wxString mime = file->GetMimeType();
            wxString path = "data:" + mime + ";base64," + data;
            //Then we can redirect the call
            webkit_network_request_set_uri(request, path.utf8_str());
        }

    }
}
*/

static gboolean
wxgtk_webview_webkit_context_menu(WebKitWebView *,
                                  WebKitContextMenu *,
                                  GdkEvent *,
                                  WebKitHitTestResult *,
                                  wxWebViewWebKit2 *ctrl)
{
    if(ctrl->IsContextMenuEnabled())
        return FALSE;
    else
        return TRUE;
}

} // extern "C"

//-----------------------------------------------------------------------------
// wxWebViewWebKit2
//-----------------------------------------------------------------------------

extern const char wxWebViewBackendWebKit2[] = "wxWebViewWebKit2";

wxIMPLEMENT_DYNAMIC_CLASS(wxWebViewWebKit2, wxWebView);

wxWebViewWebKit2::wxWebViewWebKit2()
{
    m_web_view = NULL;
}

bool wxWebViewWebKit2::Create(wxWindow *parent,
                      wxWindowID id,
                      const wxString &url,
                      const wxPoint& pos,
                      const wxSize& size,
                      long style,
                      const wxString& name)
{
    FindClear();

    // We currently unconditionally impose scrolling in both directions as it's
    // necessary to show arbitrary pages.
    style |= wxHSCROLL | wxVSCROLL;

    if (!PreCreation( parent, pos, size ) ||
        !CreateBase( parent, id, pos, size, style, wxDefaultValidator, name ))
    {
        wxFAIL_MSG( wxT("wxWebViewWebKit2 creation failed") );
        return false;
    }

    m_web_view = WEBKIT_WEB_VIEW(webkit_web_view_new());
    GTKCreateScrolledWindowWith(GTK_WIDGET(m_web_view));
    g_object_ref(m_widget);
    
    g_signal_connect_after(m_web_view, "notify::title",
                           G_CALLBACK(wxgtk_webview_webkit_title_changed), this);
/*
    g_signal_connect_after(m_web_view, "navigation-policy-decision-requested",
                           G_CALLBACK(wxgtk_webview_webkit_navigation),
                           this);
    g_signal_connect_after(m_web_view, "load-error",
                           G_CALLBACK(wxgtk_webview_webkit_error),
                           this);

    g_signal_connect_after(m_web_view, "new-window-policy-decision-requested",
                           G_CALLBACK(wxgtk_webview_webkit_new_window), this);
*/
//    g_signal_connect_after(m_web_view, "resource-request-starting",
  //                         G_CALLBACK(wxgtk_webview_webkit_resource_req), this);
  
    g_signal_connect_after(m_web_view, "context-menu",
                           G_CALLBACK(wxgtk_webview_webkit_context_menu), this);
    
    m_parent->DoAddChild( this );

    PostCreation(size);

    /* Open a webpage */
    webkit_web_view_load_uri(m_web_view, url.utf8_str());

    // last to avoid getting signal too early
    g_signal_connect_after(m_web_view, "load-changed",
                           G_CALLBACK(wxgtk_webview_webkit_load_changed),
                           this);

    return true;
}

wxWebViewWebKit2::~wxWebViewWebKit2()
{
    if (m_web_view)
        GTKDisconnect(m_web_view);
}

bool wxWebViewWebKit2::Enable( bool enable )
{
    if (!wxControl::Enable(enable))
        return false;

    gtk_widget_set_sensitive(gtk_bin_get_child(GTK_BIN(m_widget)), enable);

    //if (enable)
    //    GTKFixSensitivity();

    return true;
}

GdkWindow*
wxWebViewWebKit2::GTKGetWindow(wxArrayGdkWindows& WXUNUSED(windows)) const
{
    GdkWindow* window = gtk_widget_get_parent_window(m_widget);
    return window;
}

void wxWebViewWebKit2::SetWebkitZoom(float level)
{
    webkit_web_view_set_zoom_level(m_web_view, level);
}

float wxWebViewWebKit2::GetWebkitZoom() const
{
    return webkit_web_view_get_zoom_level(m_web_view);
}

void wxWebViewWebKit2::Stop()
{
     webkit_web_view_stop_loading(m_web_view);
}

void wxWebViewWebKit2::Reload(wxWebViewReloadFlags flags)
{
    if (flags & wxWEBVIEW_RELOAD_NO_CACHE)
    {
        webkit_web_view_reload_bypass_cache(m_web_view);
    }
    else
    {
        webkit_web_view_reload(m_web_view);
    }
}

void wxWebViewWebKit2::LoadURL(const wxString& url)
{
    webkit_web_view_load_uri(m_web_view, url);
}


void wxWebViewWebKit2::GoBack()
{
    webkit_web_view_go_back(m_web_view);
}

void wxWebViewWebKit2::GoForward()
{
    webkit_web_view_go_forward(m_web_view);
}


bool wxWebViewWebKit2::CanGoBack() const
{
    return webkit_web_view_can_go_back(m_web_view);
}


bool wxWebViewWebKit2::CanGoForward() const
{
    return webkit_web_view_can_go_forward(m_web_view);
}

void wxWebViewWebKit2::ClearHistory()
{
//    WebKitWebBackForwardList* history;
  //  history = webkit_web_view_get_back_forward_list(m_web_view);
    //webkit_web_back_forward_list_clear(history);
}

void wxWebViewWebKit2::EnableHistory(bool enable)
{
 /*   WebKitWebBackForwardList* history;
    history = webkit_web_view_get_back_forward_list(m_web_view);
    if(enable)
    {
        webkit_web_back_forward_list_set_limit(history, m_historyLimit);
    }
    else
    {
        webkit_web_back_forward_list_set_limit(history, 0);
    }*/
}

wxVector<wxSharedPtr<wxWebViewHistoryItem> > wxWebViewWebKit2::GetBackwardHistory()
{
    wxVector<wxSharedPtr<wxWebViewHistoryItem> > backhist;
/*    WebKitWebBackForwardList* history;
    history = webkit_web_view_get_back_forward_list(m_web_view);
    GList* list = webkit_web_back_forward_list_get_back_list_with_limit(history,
                                                                        m_historyLimit);
    //We need to iterate in reverse to get the order we desire
    for(int i = g_list_length(list) - 1; i >= 0 ; i--)
    {
        WebKitWebHistoryItem* gtkitem = (WebKitWebHistoryItem*)g_list_nth_data(list, i);
        wxWebViewHistoryItem* wxitem = new wxWebViewHistoryItem(
                                   webkit_web_history_item_get_uri(gtkitem),
                                   webkit_web_history_item_get_title(gtkitem));
        wxitem->m_histItem = gtkitem;
        wxSharedPtr<wxWebViewHistoryItem> item(wxitem);
        backhist.push_back(item);
    }*/
    return backhist;
}

wxVector<wxSharedPtr<wxWebViewHistoryItem> > wxWebViewWebKit2::GetForwardHistory()
{
    wxVector<wxSharedPtr<wxWebViewHistoryItem> > forwardhist;
    /*WebKitWebBackForwardList* history;
    history = webkit_web_view_get_back_forward_list(m_web_view);
    GList* list = webkit_web_back_forward_list_get_forward_list_with_limit(history,
                                                                           m_historyLimit);
    for(guint i = 0; i < g_list_length(list); i++)
    {
        WebKitWebHistoryItem* gtkitem = (WebKitWebHistoryItem*)g_list_nth_data(list, i);
        wxWebViewHistoryItem* wxitem = new wxWebViewHistoryItem(
                                   webkit_web_history_item_get_uri(gtkitem),
                                   webkit_web_history_item_get_title(gtkitem));
        wxitem->m_histItem = gtkitem;
        wxSharedPtr<wxWebViewHistoryItem> item(wxitem);
        forwardhist.push_back(item);
    }*/
    return forwardhist;
}

void wxWebViewWebKit2::LoadHistoryItem(wxSharedPtr<wxWebViewHistoryItem> item)
{
    /*WebKitWebHistoryItem* gtkitem = (WebKitWebHistoryItem*)item->m_histItem;
    if(gtkitem)
    {
        webkit_web_view_go_to_back_forward_item(m_web_view,
                                                WEBKIT_WEB_HISTORY_ITEM(gtkitem));
    }*/
}

bool wxWebViewWebKit2::CanCut() const
{
    //return webkit_web_view_can_cut_clipboard(m_web_view);
    return true;
}

bool wxWebViewWebKit2::CanCopy() const
{
    //return webkit_web_view_can_copy_clipboard(m_web_view);
    return true;
}

bool wxWebViewWebKit2::CanPaste() const
{
    //return webkit_web_view_can_paste_clipboard(m_web_view);
    return true;
}

void wxWebViewWebKit2::Cut()
{
    webkit_web_view_execute_editing_command(m_web_view,
                                            WEBKIT_EDITING_COMMAND_CUT);
}

void wxWebViewWebKit2::Copy()
{
    webkit_web_view_execute_editing_command(m_web_view,
                                            WEBKIT_EDITING_COMMAND_COPY);
}

void wxWebViewWebKit2::Paste()
{
    webkit_web_view_execute_editing_command(m_web_view,
                                            WEBKIT_EDITING_COMMAND_CUT);
}

bool wxWebViewWebKit2::CanUndo() const
{
    //return webkit_web_view_can_undo(m_web_view);
    return true;
}

bool wxWebViewWebKit2::CanRedo() const
{
    //return webkit_web_view_can_redo(m_web_view);
    return true;
}

void wxWebViewWebKit2::Undo()
{
    webkit_web_view_execute_editing_command(m_web_view,
                                            WEBKIT_EDITING_COMMAND_UNDO);
}

void wxWebViewWebKit2::Redo()
{
    webkit_web_view_execute_editing_command(m_web_view,
                                            WEBKIT_EDITING_COMMAND_REDO);
}

wxString wxWebViewWebKit2::GetCurrentURL() const
{
    // FIXME: check which encoding the web kit control uses instead of
    // assuming UTF8 (here and elsewhere too)
    return wxString::FromUTF8(webkit_web_view_get_uri(m_web_view));
}


wxString wxWebViewWebKit2::GetCurrentTitle() const
{
    return wxString::FromUTF8(webkit_web_view_get_title(m_web_view));
}


wxString wxWebViewWebKit2::GetPageSource() const
{
    //WebKitWebFrame* frame = webkit_web_view_get_main_frame(m_web_view);
    //WebKitWebDataSource* src = webkit_web_frame_get_data_source (frame);

    // TODO: check encoding with
    // const gchar*
    // webkit_web_data_source_get_encoding(WebKitWebDataSource *data_source);
    //return wxString(webkit_web_data_source_get_data (src)->str, wxConvUTF8);
    return "";
}


wxWebViewZoom wxWebViewWebKit2::GetZoom() const
{
    float zoom = GetWebkitZoom();

    // arbitrary way to map float zoom to our common zoom enum
    if (zoom <= 0.65)
    {
        return wxWEBVIEW_ZOOM_TINY;
    }
    else if (zoom > 0.65 && zoom <= 0.90)
    {
        return wxWEBVIEW_ZOOM_SMALL;
    }
    else if (zoom > 0.90 && zoom <= 1.15)
    {
        return wxWEBVIEW_ZOOM_MEDIUM;
    }
    else if (zoom > 1.15 && zoom <= 1.45)
    {
        return wxWEBVIEW_ZOOM_LARGE;
    }
    else if (zoom > 1.45)
    {
        return wxWEBVIEW_ZOOM_LARGEST;
    }

    // to shut up compilers, this can never be reached logically
    wxASSERT(false);
    return wxWEBVIEW_ZOOM_MEDIUM;
}


void wxWebViewWebKit2::SetZoom(wxWebViewZoom zoom)
{
    // arbitrary way to map our common zoom enum to float zoom
    switch (zoom)
    {
        case wxWEBVIEW_ZOOM_TINY:
            SetWebkitZoom(0.6f);
            break;

        case wxWEBVIEW_ZOOM_SMALL:
            SetWebkitZoom(0.8f);
            break;

        case wxWEBVIEW_ZOOM_MEDIUM:
            SetWebkitZoom(1.0f);
            break;

        case wxWEBVIEW_ZOOM_LARGE:
            SetWebkitZoom(1.3);
            break;

        case wxWEBVIEW_ZOOM_LARGEST:
            SetWebkitZoom(1.6);
            break;

        default:
            wxASSERT(false);
    }
}

void wxWebViewWebKit2::SetZoomType(wxWebViewZoomType type)
{
    //webkit_web_view_set_full_content_zoom(m_web_view,
      //                                    (type == wxWEBVIEW_ZOOM_TYPE_LAYOUT ?
        //                                  TRUE : FALSE));
}

wxWebViewZoomType wxWebViewWebKit2::GetZoomType() const
{
   // gboolean fczoom = webkit_web_view_get_full_content_zoom(m_web_view);
///
   // if (fczoom) return wxWEBVIEW_ZOOM_TYPE_LAYOUT;
   /* else*/        return wxWEBVIEW_ZOOM_TYPE_TEXT;
}

bool wxWebViewWebKit2::CanSetZoomType(wxWebViewZoomType) const
{
    // this port supports all zoom types
    return true;
}

void wxWebViewWebKit2::DoSetPage(const wxString& html, const wxString& baseUri)
{
    webkit_web_view_load_html(m_web_view,
                              html.mb_str(wxConvUTF8),
                              baseUri.mb_str(wxConvUTF8));
}

void wxWebViewWebKit2::Print()
{
    //WebKitWebFrame* frame = webkit_web_view_get_main_frame(m_web_view);
    //webkit_web_frame_print (frame);

    // GtkPrintOperationResult  webkit_web_frame_print_full
    //      (WebKitWebFrame *frame,
    //       GtkPrintOperation *operation,
    //       GtkPrintOperationAction action,
    //       GError **error);

}


bool wxWebViewWebKit2::IsBusy() const
{
   // return webkit_web_view_is_loading(m_web_view);
}

void wxWebViewWebKit2::SetEditable(bool enable)
{
//    webkit_web_view_set_editable(m_web_view, enable);
}

bool wxWebViewWebKit2::IsEditable() const
{
//    return webkit_web_view_get_editable(m_web_view);
    return false;
}

void wxWebViewWebKit2::DeleteSelection()
{
//    webkit_web_view_delete_selection(m_web_view);
}

bool wxWebViewWebKit2::HasSelection() const
{
//    return webkit_web_view_has_selection(m_web_view);
    return false;
}

void wxWebViewWebKit2::SelectAll()
{
    //webkit_web_view_select_all(m_web_view);
}

wxString wxWebViewWebKit2::GetSelectedText() const
{
   /* WebKitDOMDocument* doc;
    WebKitDOMDOMWindow* win;
    WebKitDOMDOMSelection* sel;
    WebKitDOMRange* range;

    doc = webkit_web_view_get_dom_document(m_web_view);
    win = webkit_dom_document_get_default_view(WEBKIT_DOM_DOCUMENT(doc));
    sel = webkit_dom_dom_window_get_selection(WEBKIT_DOM_DOM_WINDOW(win));
    range = webkit_dom_dom_selection_get_range_at(WEBKIT_DOM_DOM_SELECTION(sel),
                                                  0, NULL);
    return wxString(webkit_dom_range_get_text(WEBKIT_DOM_RANGE(range)),
                    wxConvUTF8);*/
    return "";
}

wxString wxWebViewWebKit2::GetSelectedSource() const
{
    /*WebKitDOMDocument* doc;
    WebKitDOMDOMWindow* win;
    WebKitDOMDOMSelection* sel;
    WebKitDOMRange* range;
    WebKitDOMElement* div;
    WebKitDOMDocumentFragment* clone;
    WebKitDOMHTMLElement* html;

    doc = webkit_web_view_get_dom_document(m_web_view);
    win = webkit_dom_document_get_default_view(WEBKIT_DOM_DOCUMENT(doc));
    sel = webkit_dom_dom_window_get_selection(WEBKIT_DOM_DOM_WINDOW(win));
    range = webkit_dom_dom_selection_get_range_at(WEBKIT_DOM_DOM_SELECTION(sel),
                                                  0, NULL);
    div = webkit_dom_document_create_element(WEBKIT_DOM_DOCUMENT(doc), "div", NULL);

    clone = webkit_dom_range_clone_contents(WEBKIT_DOM_RANGE(range), NULL);
    webkit_dom_node_append_child(&div->parent_instance, &clone->parent_instance, NULL);
    html = (WebKitDOMHTMLElement*)div;

    return wxString(webkit_dom_html_element_get_inner_html(WEBKIT_DOM_HTML_ELEMENT(html)),
                    wxConvUTF8);*/
    return "";
}

void wxWebViewWebKit2::ClearSelection()
{
    /*WebKitDOMDocument* doc;
    WebKitDOMDOMWindow* win;
    WebKitDOMDOMSelection* sel;

    doc = webkit_web_view_get_dom_document(m_web_view);
    win = webkit_dom_document_get_default_view(WEBKIT_DOM_DOCUMENT(doc));
    sel = webkit_dom_dom_window_get_selection(WEBKIT_DOM_DOM_WINDOW(win));
    webkit_dom_dom_selection_remove_all_ranges(WEBKIT_DOM_DOM_SELECTION(sel));
*/
}

wxString wxWebViewWebKit2::GetPageText() const
{
    /*WebKitDOMDocument* doc;
    WebKitDOMHTMLElement* body;

    doc = webkit_web_view_get_dom_document(m_web_view);
    body = webkit_dom_document_get_body(WEBKIT_DOM_DOCUMENT(doc));
    return wxString(webkit_dom_html_element_get_inner_text(WEBKIT_DOM_HTML_ELEMENT(body)),
                    wxConvUTF8);*/
    return "";
}

void wxWebViewWebKit2::RunScript(const wxString& javascript)
{
    //webkit_web_view_execute_script(m_web_view,
      //                             javascript.mb_str(wxConvUTF8));
}

void wxWebViewWebKit2::RegisterHandler(wxSharedPtr<wxWebViewHandler> handler)
{
    m_handlerList.push_back(handler);
}

void wxWebViewWebKit2::EnableContextMenu(bool enable)
{
#if !WEBKIT_CHECK_VERSION(1, 10, 0) //If we are using an older version
    g_object_set(webkit_web_view_get_settings(m_web_view), 
                 "enable-default-context-menu", enable, NULL);
#endif
    wxWebView::EnableContextMenu(enable);
}

long wxWebViewWebKit2::Find(const wxString& text, int flags)
{
/*    bool newSearch = false;
    if(text != m_findText || 
       (flags & wxWEBVIEW_FIND_MATCH_CASE) != (m_findFlags & wxWEBVIEW_FIND_MATCH_CASE))
    {
        newSearch = true;
        //If it is a new search we need to clear existing highlights
        webkit_web_view_unmark_text_matches(m_web_view);
        webkit_web_view_set_highlight_text_matches(m_web_view, false);
    }

    m_findFlags = flags;
    m_findText = text;

    //If the search string is empty then we clear any selection and highlight
    if(text == "")
    {
        webkit_web_view_unmark_text_matches(m_web_view);
        webkit_web_view_set_highlight_text_matches(m_web_view, false);
        ClearSelection();
        return wxNOT_FOUND;
    }

    bool wrap = false, matchCase = false, forward = true;
    if(flags & wxWEBVIEW_FIND_WRAP)
        wrap = true;
    if(flags & wxWEBVIEW_FIND_MATCH_CASE)
        matchCase = true;
    if(flags & wxWEBVIEW_FIND_BACKWARDS)
        forward = false;

    if(newSearch)
    {
        //Initially we mark the matches to know how many we have
        m_findCount = webkit_web_view_mark_text_matches(m_web_view, wxGTK_CONV(text), matchCase, 0);
        //In this case we return early to match IE behaviour
        m_findPosition = -1;
        return m_findCount;
    }
    else
    {
        if(forward)
            m_findPosition++;
        else
            m_findPosition--;
        if(m_findPosition < 0)
            m_findPosition += m_findCount;
        if(m_findPosition > m_findCount)
            m_findPosition -= m_findCount;
    }

    //Highlight them if needed
    bool highlight = flags & wxWEBVIEW_FIND_HIGHLIGHT_RESULT ? true : false;
    webkit_web_view_set_highlight_text_matches(m_web_view, highlight);     

    if(!webkit_web_view_search_text(m_web_view, wxGTK_CONV(text), matchCase, forward, wrap))
    {
        m_findPosition = -1;
        ClearSelection();
        return wxNOT_FOUND;
    }
    wxLogMessage(wxString::Format("Returning %d", m_findPosition));
    return newSearch ? m_findCount : m_findPosition;*/
    return 0;
}

void wxWebViewWebKit2::FindClear()
{
    m_findCount = 0;
    m_findFlags = 0;
    m_findText = "";
    m_findPosition = -1;
}

// static
wxVisualAttributes
wxWebViewWebKit2::GetClassDefaultAttributes(wxWindowVariant WXUNUSED(variant))
{
     return GetDefaultAttributesFromGTKWidget(webkit_web_view_new());
}


#endif // wxUSE_WEBVIEW && wxUSE_WEBVIEW_WEBKIT
