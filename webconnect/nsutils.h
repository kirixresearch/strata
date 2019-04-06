/*!
 *
 * Copyright (c) 2006-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  wxWebConnect Embedded Web Browser Control Library
 * Author:   Benjamin I. Williams
 * Created:  2006-10-08
 *
 */


#ifndef __WXWEBCONNECT_NSUTILS_H
#define __WXWEBCONNECT_NSUTILS_H


wxString ns2wx(nsEmbedCString& str);
wxString ns2wx(nsEmbedString& str);
wxString ns2wx(const char16_t* str);
void wx2ns(const wxString& wxstr, nsEmbedString& nsstr);
void wx2ns(const wxString& wxstr, nsEmbedCString& nsstr);
char16_t* wxToUnichar(const wxString& wxstr);
void freeUnichar(char16_t* p);


ns_smartptr<nsIWindowWatcher> nsGetWindowWatcherService();
ns_smartptr<nsIPrefBranch> nsGetPrefBranch();
ns_smartptr<nsIProperties> nsGetDirectoryService();
ns_smartptr<nsISupports> nsGetService(const char* contract_id);
ns_smartptr<nsISupports> nsCreateInstance(const char* contract_id);
ns_smartptr<nsISupports> nsCreateInstance(const nsCID& cid);
ns_smartptr<nsILocalFile> nsNewLocalFile(const wxString& filename);
ns_smartptr<nsIURI> nsNewURI(const wxString& spec);




class wxWebProgressBase;




class ProgressListenerAdaptor : public nsITransfer
{
public:

    NS_DECL_ISUPPORTS

    ProgressListenerAdaptor(wxWebProgressBase* progress);
    ~ProgressListenerAdaptor();
    
    void ClearProgressReference()
    {
        m_progress = NULL;
    }

    NS_IMETHODIMP Init(nsIURI* source,
                       nsIURI* target,
                       const nsAString& display_name,
                       nsIMIMEInfo* mime_info,
                       PRTime start_time,
                       nsILocalFile* temp_file,
                       nsICancelable* cancelable,
                       bool is_private);

    NS_IMETHOD SetSha256Hash(const nsACString& hash);

    NS_IMETHOD OnStateChange(nsIWebProgress* web_progress, 
                             nsIRequest* request,
                             PRUint32 state_flags, 
                             nsresult status);
                             
    NS_IMETHOD OnProgressChange(nsIWebProgress* web_progress,
                                nsIRequest* request,
                                PRInt32 cur_self_progress,
                                PRInt32 max_self_progress,
                                PRInt32 cur_total_progress,
                                PRInt32 max_total_progress);
    
    NS_IMETHOD OnProgressChange64(
                                 nsIWebProgress* web_progress,
                                 nsIRequest* request,
                                 PRInt64 cur_self_progress,
                                 PRInt64 max_self_progress,
                                 PRInt64 cur_total_progress,
                                 PRInt64 max_total_progress);
    NS_IMETHOD OnLocationChange(
                             nsIWebProgress* web_progress,
                             nsIRequest* request,
                             nsIURI* location,
                             uint32_t flags);

    NS_IMETHOD OnStatusChange(
                             nsIWebProgress* web_progress,
                             nsIRequest* request,
                             nsresult status,
                             const char16_t* message);

    NS_IMETHOD OnSecurityChange(
                             nsIWebProgress* web_progress,
                             nsIRequest* request,
                             PRUint32 state);
                             
    NS_IMETHOD OnRefreshAttempted(
                            nsIWebProgress* web_progress,
                            nsIURI* refresh_uri,
                            PRInt32 millis,
                            bool same_uri,
                            bool *retval);

public:

    wxWebProgressBase* m_progress;
};


nsIWebProgressListener* CreateProgressListenerAdaptor(wxWebProgressBase* progress);


#endif

