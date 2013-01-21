/*!
 *
 * Copyright (c) 2007-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   Benjamin I. Williams
 * Created:  2007-12-24
 *
 */


#ifndef __APP_FEEDPARSER_H
#define __APP_FEEDPARSER_H



struct FeedItem
{
    wxString feed_title;
    wxString feed_language;
    wxString feed_link;
    wxString feed_description;

    wxString title;
    wxString link;
    wxString summary;
    wxString description;
    wxString guid;
    wxString publish_date;
};


class FeedParser
{

public:

    FeedParser();

    bool loadAndParse(const wxString& filename);
    const std::vector<FeedItem>& getItems() const;
    tango::ISetPtr convertToSet();
    
private:

    void parseRSS1Feed(kl::xmlnode& channel);
    void parseRSS2Feed(kl::xmlnode& channel);
    void parseRSSEntry(kl::xmlnode& item_node);
    void parseAtomFeed(kl::xmlnode& channel);
    void parseAtomEntry(kl::xmlnode& item_node);
    
private:

    std::vector<FeedItem> m_items;
};


#endif
