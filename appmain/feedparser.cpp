/*!
 *
 * Copyright (c) 2007-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   Benjamin I. Williams
 * Created:  2007-03-24
 *
 */


#include "appmain.h"
#include "feedparser.h"
#include "../kscript/kscript.h"
#include "../kscript/jsdate.h"
#include <wx/html/htmlpars.h>


FeedParser::FeedParser()
{
}

bool FeedParser::loadAndParse(const wxString& filename)
{
    kl::xmlnode root;
    if (!root.load(towstr(filename), kl::xmlnode::parseRelaxed))
        return false;
 
    m_items.clear();
    
    std::wstring tag_name = root.getNodeName();
    kl::makeLower(tag_name);
    
    if (tag_name == L"rdf:rdf")
    {
        // RSS 1.0
        parseRSS1Feed(root);
    }
     else if (tag_name == L"rss")
    {
        // RSS 2.0
        size_t i, channel_count = root.getChildCount();
        
        for (i = 0; i < channel_count; ++i)
        {
            kl::xmlnode& channel_node = root.getChild(i);
            parseRSS2Feed(channel_node);
        }
    }
     else if (tag_name == L"feed")
    {
        // ATOM
        parseAtomFeed(root);
    }
    
    return true;
}

const std::vector<FeedItem>& FeedParser::getItems() const
{
    return m_items;
}



void FeedParser::parseRSS1Feed(kl::xmlnode& channel)
{
    size_t i, child_count = channel.getChildCount();
    
    for (i = 0; i < child_count; ++i)
    {
        kl::xmlnode& node = channel.getChild(i);
        std::wstring name = node.getNodeName();
        kl::makeLower(name);
        
        if (name == L"item")
        {
            parseRSSEntry(node);
        }
    }
}

void FeedParser::parseRSS2Feed(kl::xmlnode& channel)
{
    size_t i, child_count = channel.getChildCount();
    
    for (i = 0; i < child_count; ++i)
    {
        kl::xmlnode& node = channel.getChild(i);
        std::wstring name = node.getNodeName();
        kl::makeLower(name);
        
        if (name == L"item")
        {
            parseRSSEntry(node);
        }
    }
}

void FeedParser::parseRSSEntry(kl::xmlnode& item_node)
{
    kl::xmlnode& title_node = item_node.getChild(L"title");
    kl::xmlnode& link_node = item_node.getChild(L"link");
    kl::xmlnode& description_node = item_node.getChild(L"description");
    kl::xmlnode& dcdate_node = item_node.getChild(L"dc:date"); // used by some RSS 1.0
    kl::xmlnode& pubdate_node = item_node.getChild(L"pubDate");
    kl::xmlnode& guid_node = item_node.getChild(L"guid");
    
    FeedItem item;
    if (!title_node.isEmpty())
        item.title = towx(title_node.getNodeValue());
    if (!link_node.isEmpty())
        item.link = towx(link_node.getNodeValue());
    if (!description_node.isEmpty())
        item.description = towx(description_node.getNodeValue());
    if (!dcdate_node.isEmpty())
        item.publish_date = towx(dcdate_node.getNodeValue());
    if (!pubdate_node.isEmpty())
        item.publish_date = towx(pubdate_node.getNodeValue());
    if (!guid_node.isEmpty())
        item.guid = towx(guid_node.getNodeValue());
    
    if (item.guid.IsEmpty())
    {
        // RSS 1.0
        if (item_node.getPropertyIdx(L"rdf:about") != -1)
        {
            item.guid = item_node.getProperty(L"rdf:about").value;
        }
    }
    
    item.guid.Trim(true);
    item.guid.Trim(false);
    item.publish_date.Trim(true);
    item.publish_date.Trim(false);
    item.link.Trim(true);
    item.link.Trim(false);
    item.summary.Trim(true);
    item.summary.Trim(false);
    item.description.Trim(true);
    item.description.Trim(false);
    
    m_items.push_back(item);
}


void FeedParser::parseAtomFeed(kl::xmlnode& channel)
{
    size_t i, child_count = channel.getChildCount();
    
    for (i = 0; i < child_count; ++i)
    {
        kl::xmlnode& node = channel.getChild(i);
        std::wstring name = node.getNodeName();
        kl::makeLower(name);
        
        if (name == L"entry")
        {
            parseAtomEntry(node);
        }
    }
}

void FeedParser::parseAtomEntry(kl::xmlnode& item_node)
{
    kl::xmlnode& title_node = item_node.getChild(L"title");
    kl::xmlnode& link_node = item_node.getChild(L"link");
    kl::xmlnode& content_node = item_node.getChild(L"content");
    kl::xmlnode& summary_node = item_node.getChild(L"summary");
    kl::xmlnode& updated_node = item_node.getChild(L"updated");
    kl::xmlnode& id_node = item_node.getChild(L"id");
    
    FeedItem item;
    if (!title_node.isEmpty())
        item.title = towx(title_node.getNodeValue());
    if (!link_node.isEmpty())
    {
        kl::xmlproperty& rel_prop = link_node.getProperty(L"rel");
        kl::xmlproperty& href_prop = link_node.getProperty(L"href");
        
        // there are lots of link types in atom; we'll prefer the 'alternate'
        // type of href, but if there isn't one of that kind, we'll take any other
        
        if (item.link.empty())
            item.link = towx(href_prop.value);
        if (rel_prop.isEmpty() || rel_prop.value == L"alternate")
            item.link = towx(href_prop.value);
    }
    if (!content_node.isEmpty())
        item.description = towx(content_node.getNodeValue());
    if (!summary_node.isEmpty())
        item.summary = towx(summary_node.getNodeValue());
    if (!updated_node.isEmpty())
        item.publish_date = towx(updated_node.getNodeValue());
    if (!id_node.isEmpty())
        item.guid = towx(id_node.getNodeValue());
    
    item.guid.Trim(true);
    item.guid.Trim(false);
    item.publish_date.Trim(true);
    item.publish_date.Trim(false);
    item.link.Trim(true);
    item.link.Trim(false);
    item.summary.Trim(true);
    item.summary.Trim(false);
    item.description.Trim(true);
    item.description.Trim(false);

    m_items.push_back(item);
}


static wxString StripHTML(const wxString& str)
{
    wxString temps = str;
    temps.MakeUpper();
    if (temps.Find(wxT("<P>")) == -1 &&
        temps.Find(wxT("<A ")) == -1 &&
        temps.Find(wxT("<BR")) == -1 &&
        temps.Find(wxT("<IMG")) == -1 &&
        temps.Find(wxT("&#")) == -1 &&
        temps.Find(wxT("&LG")) == -1 &&
        temps.Find(wxT("&GT")))
    {
        // string is probably pretty clean, pass it through
        return str;
    }
    
    wxString result;
    result.reserve(str.length());

    wxChar quote_char = 0;
   
    const wxChar *p = str.c_str();
   
    while (*p)
    {
        if (*p == wxT('<'))
        {
            while (1)
            {
                if (!*p)
                    break;
                if (*p == '"' || *p == '\'')
                    quote_char = *p;
                if (*p == quote_char)
                    quote_char = 0;
                if (*p == wxT('>'))
                    break;
                ++p;
            }
        }
         else
        {
            result += *p;
        }
        
        ++p;
    }

    wxHtmlEntitiesParser entities;
    return entities.Parse(result);
}
  

tango::ISetPtr FeedParser::convertToSet()
{
    tango::IDatabasePtr db = g_app->getDatabase();
    if (db.isNull())
        return xcm::null;
    
    tango::ISetPtr dest_set;
    
    size_t max_link_len = 1;
    size_t max_title_len = 1;
    size_t max_description_len = 1;
    size_t max_id_len = 1;
    
    const std::vector<FeedItem>& items = getItems();
    size_t i, item_count = items.size();
    
    for (i = 0; i < item_count; ++i)
    {
        FeedItem item = items[i];
        max_link_len = wxMax(max_link_len, item.link.Length());
        max_title_len = wxMax(max_title_len, item.title.Length());
        max_description_len = wxMax(max_description_len, item.description.Length());
        max_description_len = wxMax(max_description_len, item.summary.Length());
        max_id_len = wxMax(max_id_len, item.guid.Length());
    }

    // expand by 50%
    max_link_len = (max_link_len*150)/100;
    max_title_len = (max_title_len*150)/100;
    max_description_len = (max_description_len*150)/100;
    max_id_len = (max_id_len*150)/100;

    // create the output file
    tango::IStructurePtr structure = g_app->getDatabase()->createStructure();
    tango::IColumnInfoPtr colinfo;
    
    colinfo = structure->createColumn();
    colinfo->setName(L"link");
    colinfo->setType(tango::typeWideCharacter);
    colinfo->setWidth(max_link_len);
    colinfo->setScale(0);
    
    colinfo = structure->createColumn();
    colinfo->setName(L"title");
    colinfo->setType(tango::typeWideCharacter);
    colinfo->setWidth(max_title_len);
    colinfo->setScale(0);
    
    colinfo = structure->createColumn();
    colinfo->setName(L"description");
    colinfo->setType(tango::typeWideCharacter);
    colinfo->setWidth(max_description_len);
    colinfo->setScale(0);
    
    colinfo = structure->createColumn();
    colinfo->setName(L"publication_date");
    colinfo->setType(tango::typeDateTime);
    colinfo->setWidth(8);
    colinfo->setScale(0);
    
    colinfo = structure->createColumn();
    colinfo->setName(L"id");
    colinfo->setType(tango::typeWideCharacter);
    colinfo->setWidth(max_id_len);
    colinfo->setScale(0);

    dest_set = db->createSet(L"", structure, NULL);

    if (dest_set.isNull())
        return xcm::null;

    tango::IRowInserterPtr row_inserter = dest_set->getRowInserter();
    if (row_inserter.isNull())
        return xcm::null;
        
    row_inserter->startInsert(L"");
    
    tango::objhandle_t link_handle = row_inserter->getHandle(L"link");
    tango::objhandle_t title_handle = row_inserter->getHandle(L"title");
    tango::objhandle_t description_handle = row_inserter->getHandle(L"description");
    tango::objhandle_t pubdate_handle = row_inserter->getHandle(L"publication_date");
    tango::objhandle_t id_handle = row_inserter->getHandle(L"id");


    for (i = 0; i < item_count; ++i)
    {
        FeedItem item = items[i];


        wxString desc;
        desc = item.description;
        if (desc.IsEmpty())
            desc = item.summary;
            

        row_inserter->putWideString(link_handle, towstr(item.link));
        row_inserter->putWideString(title_handle, towstr(StripHTML(item.title)));
        row_inserter->putWideString(description_handle, towstr(StripHTML(desc)));
        row_inserter->putWideString(id_handle, towstr(item.guid));
        
        if (item.publish_date.Length() > 0)
        {
            // parseJsDateTime is an internal function to kscript,
            // but it does exactly what we need.  Eventually we should
            // have a full date time parser in a kl, but I didn't want
            // to put it in two places right now (because we're still
            // constantly adding new date formats to parseJsDateTime)
            
            std::wstring dts = towstr(item.publish_date);
            int yy = 0, mm = 0, dd = 0, h = 0, m = 0, s = 0;
            bool success = kscript::parseJsDateTime(dts.c_str(),
                              &yy, &mm, &dd, &h, &m, &s);
                              
            if (success)
            {
                row_inserter->putDateTime(pubdate_handle, 
                                tango::DateTime(yy, mm, dd, h, m, s));
            }
             else
            {
                row_inserter->putDateTime(pubdate_handle, 0);
            }
        } 
         else
        {
            row_inserter->putDateTime(pubdate_handle, 0);
        }
       
        row_inserter->insertRow();
    }

    row_inserter->finishInsert();

    return dest_set;
}

