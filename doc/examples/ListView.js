

//(EXAMPLE)
//Description: Example 1
//Class: ListView
//Code:


class MyForm extends Form
{
    var m_list;
    var m_text;
    
    var m_last_sort_order;
    var m_last_sort_column;
    
    function MyForm()
    {
        super("ListView Test", 100, 100, 700, 520);
        setMinSize(500, 360);
        
        // create list view and add some columns        
        m_list = new ListView;
        m_list.setView(ListView.Details);
        m_list.addColumn("Test Column", 200, ListView.AlignLeft);
        m_list.addColumn("Test Column 2", 200, ListView.AlignLeft);
        m_list.addColumn("Test Column 3", 200, ListView.AlignLeft);
        this.add(m_list);
        
        // add some items
        var folder_bitmap = getFolderBitmap();
        m_list.addItem(new ListViewItem("Finch", folder_bitmap));
        m_list.addItem(new ListViewItem("Bluebird", folder_bitmap));
        m_list.addItem(new ListViewItem("Goose", folder_bitmap));
        m_list.addItem(new ListViewItem("Sparrow", folder_bitmap));
        m_list.addItem(new ListViewItem("Hummingbird", folder_bitmap));
        m_list.addItem(new ListViewItem("Duck", folder_bitmap));
        
        // connect events
        m_list.keyDown.connect(this, onKeyDown);
        m_list.itemSelect.connect(this, onItemSelect);
        m_list.itemActivate.connect(this, onItemActivate);
        m_list.itemRightClick.connect(this, onItemRightClick);
        m_list.itemBeginLabelEdit.connect(this, onItemBeginLabelEdit);
        m_list.itemEndLabelEdit.connect(this, onItemEndLabelEdit);
        m_list.columnClick.connect(this, onColumnClick);
        m_list.columnRightClick.connect(this, onColumnRightClick);
               
        // text box for event output
        m_text = new TextBox("", 0, 0, 140, 140);
        m_text.setMultiline();

        // -- some extra buttons --
        
        var additem_button = new Button("Add Item");
        additem_button.click.connect(this, onAddItem);
        
        var changeitem_button = new Button("Change Item");
        changeitem_button.click.connect(this, onChangeItem);
      
        var deleteitem_button = new Button("Delete Item");
        deleteitem_button.click.connect(this, onDeleteItem);
        
        var deleteall_button = new Button("Delete All");
        deleteall_button.click.connect(this, onDeleteAll);
        
        var exit_button = new Button("Exit");
        exit_button.click.connect(this, onExit);
        
        // these variables save the last sort order
        m_last_sort_order = false;
        m_last_sort_column = -1;
        
        // -- create button layout --
        var button_layout = new BoxLayout(Layout.Horizontal);
        button_layout.add(additem_button);
        button_layout.addSpacer(8);
        button_layout.add(changeitem_button);
        button_layout.addSpacer(8);
        button_layout.add(deleteitem_button);
        button_layout.addSpacer(8);
        button_layout.add(deleteall_button);
        button_layout.addStretchSpacer();
        button_layout.add(exit_button);
        
        // -- create main layout --
        var main_layout = new BoxLayout(Layout.Vertical);
        main_layout.addSpacer(8);
        main_layout.add(m_list, 1, Layout.Expand | Layout.Left | Layout.Right, 8);
        main_layout.addSpacer(8);
        main_layout.add(m_text, 0, Layout.Expand | Layout.Left | Layout.Right, 8);
        main_layout.addSpacer(8);
        main_layout.add(button_layout, 0, Layout.Expand | Layout.Left | Layout.Right, 8);
        main_layout.addSpacer(8);
        
        setLayout(main_layout);
    }
    
    function onColumnClick(sender, event_args)
    {
        var column_to_sort = event_args.index;
        var sort_order;
        
        if (column_to_sort == m_last_sort_column)
        {
            sort_order = m_last_sort_order ? false : true;
        }
         else
        {
            sort_order = true; // ascending
        }
        
        m_list.sortItems(column_to_sort, sort_order);
        
        m_last_sort_order = sort_order;
        m_last_sort_column = column_to_sort;
    }
    
    function onColumnRightClick(sender, event_args)
    {
        addLogLine("Column right clicked; index = " + event_args.index);
        
        var sortasc_item = new MenuItem("Sort Ascending");
        var sortdec_item = new MenuItem("Sort Descending");
        var deletecol_item = new MenuItem("Delete Column");
        
        sortasc_item.index = event_args.index;
        sortasc_item.sort_order = true;
        sortasc_item.click.connect(this, onDoColumnSort);
        
        sortasc_item.index = event_args.index;
        sortdec_item.sort_order = false;
        sortdec_item.click.connect(this, onDoColumnSort);
        
        deletecol_item.index = event_args.index;
        deletecol_item.click.connect(this, onDoColumnDelete);
        
        var menu = new Menu;
        menu.add(sortasc_item);
        menu.add(sortdec_item);
        menu.addSeparator();
        menu.add(deletecol_item);
        
        menu.popup(m_list);
    }
    
    function onDoColumnSort(sender, event_args)
    {
        m_list.sortItems(sender.index, sender.sort_order);
    }
    
    function onDoColumnDelete(sender, event_args)
    {
        m_list.deleteColumn(sender.index);
    }
    
    function onKeyDown(sender, event_args)
    {
        addLogLine("Key down; Char code = " + event_args.keyCode + 
                    " shiftDown(" + event_args.shiftDown + ")" +
                    " controlDown(" + event_args.controlDown + ")" +
                    " altDown(" + event_args.altDown + ")");
        
        // handle user pressing Del key
        if (event_args.keyCode == 127)
        {
            m_list.deleteItem(event_args.index);
        }
    }
    
    function onItemSelect(sender, event_args)
    {
        addLogLine("Item selected; index = " + event_args.index);
    }
    
    function onItemActivate(sender, event_args)
    {
        addLogLine("Item activated; index = " + event_args.index);
    }
    
    function onItemRightClick(sender, event_args)
    {
        addLogLine("Item right clicked; index = " + event_args.index);
        
        var menu_item1 = new MenuItem("Test Menu Item 1");
        var menu_item2 = new MenuItem("Test Menu Item 2");
        var menu_item3 = new MenuItem("Delete Listview Item");
        
        menu_item3.item_number = event_args.index;
        menu_item3.click.connect(this, onRightClickDelete);
        
        var menu = new Menu;
        menu.add(menu_item1);
        menu.add(menu_item2);
        menu.addSeparator();
        menu.add(menu_item3);
        
        menu.popup(m_list);
    }
    
    function onRightClickDelete(sender, event_args)
    {
        m_list.deleteItem(sender.item_number);
    }
    
    function onItemBeginLabelEdit(sender, event_args)
    {
        addLogLine("Begin label edit; index = " + event_args.index);
    }
    
    function onItemEndLabelEdit(sender, event_args)
    {
        addLogLine("End label edit; index = " + event_args.index);
    }
    
    function onAddItem(sender, event_args)
    {
        var folder_bitmap = getFolderBitmap();
        m_list.addItem(new ListViewItem("Test Item " + (m_list.getItemCount()+1),
                                        folder_bitmap));
    }
    
    function onDeleteItem(sender, event_args)
    {
        var items = m_list.getSelectedItems();
        for (var i in items)
        {
            m_list.deleteItem(items[i]);
        }
    }
    
    function onChangeItem(sender, event_args)
    {
        var items = m_list.getSelectedItems();
        var counter = 0;
        for (var i in items)
        {
            counter++;
            items[i].setText(items[i].getText() + " Changed Text");
            items[i].setColumnText(1, "Hello " + counter);
            items[i].setColumnText(2, "Hello " + counter);
        } 
    }
    
    function onDeleteAll(sender, event_args)
    {
        m_list.clear();
    }
    
    function addLogLine(s)
    {
        var date = new Date();
        
        var text = m_text.getText();
        text += date.toString() + " - " + s;
        text += "\n";
        m_text.setText(text);
    }

    function onExit()
    {
        Application.exit();
    }
};




var form = new MyForm;
form.show();
Application.run();




var g_folder_bitmap;

function getFolderBitmap()
{
    if (g_folder_bitmap)
        return g_folder_bitmap;
    
    g_folder_bitmap = new Bitmap('
        "16 16 6 1",
        "  c None",
        "! c #000000",
        "# c #808080",
        "$ c #C0C0C0",
        "% c #FFFF00",
        "& c #FFFFFF",
        "                ",
        "                ",
        "  #####         ",
        " #%$%$%#        ",
        "#%$%$%$%######  ",
        "#&&&&&&&&&&&&#! ",
        "#&%$%$%$%$%$%#! ",
        "#&$%$%$%$%$%$#! ",
        "#&%$%$%$%$%$%#! ",
        "#&$%$%$%$%$%$#! ",
        "#&%$%$%$%$%$%#! ",
        "#&$%$%$%$%$%$#! ",
        "#&%$%$%$%$%$%#! ",
        "##############! ",
        " !!!!!!!!!!!!!! ",
        "                "');
    
    return g_folder_bitmap;
}


//(/EXAMPLE)

