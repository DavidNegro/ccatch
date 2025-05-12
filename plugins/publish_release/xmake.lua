task("publish_release")
    set_category("plugin")
    on_run("main")

    -- set the menu options, but we put empty options now.
    set_menu {
                -- usage
                usage = "xmake publish_release"

                -- description
            ,   description = "Requires gh command line tool. Creates a github release"

                -- options
            ,   options = {}
            }
