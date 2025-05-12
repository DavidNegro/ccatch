set_project("unit")
set_version("0.0.1")

add_plugindirs("plugins")

add_includedirs(".")

-- unit_amalgamte
-- generates the amalgamated unit.h header
target("unit_amalgamate")
    set_kind("binary")
    add_files("unit/amalgamate.c")
    add_headerfiles("unit/buffer.i", "unit/buffer.i")

-- unit_header
-- headeronly target generating the header-file on the before_build hook.
target("unit_header")
    set_kind("headeronly")
    add_deps("unit_amalgamate", {inherit = false})
    on_load(function (target)
        target:add(
            "headerfiles",
            target:targetdir() .. "/release/unit.h",
            {always_added = true,  public = true}
        )
        target:add(
            "includedirs",
            target:targetdir() .. "/release/",
            {public = true}
        )
    end)
    before_build(function (target, opt)
        -- generate the header file
        import("core.project.depend")
        import("utils.progress")


        os.mkdir(target:targetdir())
        os.mkdir(path.join(target:targetdir(), "release"))
        local targetfile = path.join(target:targetdir(), "release", "unit.h")
        depend.on_changed(function ()
            os.vrunv(target:dep("unit_amalgamate"):targetfile(), {targetfile})
            progress.show(
                opt.progress,
                "${color.build.object}generating amalagamted header %s",
                targetfile
            )
        end, {files = os.files("unit/*.i")})
    end)

-- List of platforms
local platforms = {
    "unit_platforms/unit_platform_console.h"
}

-- platform implementations.
-- generate a target for each platform. The generated target is an static lib
-- created using a generated c file including unit.h and the platform header
-- see `unit_platforms/unit_platform_impl.c.template`

-- file to use as a template for generating the c file
local template_file = "unit_platforms/unit_platform_impl.c.template"
for _, filepath in ipairs(platforms) do
    local name = path.basename(filepath)
    local impl_name = "$(builddir)/impl/" .. name .. "_impl.c"
    target(path.basename(filepath))
        set_kind("static")
        add_deps("unit_header", {inherit = true, public = true})
        add_files(impl_name, {always_added = true})
        add_includedirs(path.directory(filepath))
        add_headerfiles(filepath, {private = true})
        before_build(function (target, opt)
            -- generate the implementation file
            import("core.project.depend")
            import("utils.progress")
            depend.on_changed(function ()
                progress.show(
                    opt.progress,
                    "${color.build.object}generating platform implementation %s",
                    impl_name
                )
                local include_template = io.readfile(template_file)
                io.writefile(impl_name, string.format(include_template, name))
            end, {files = template_file})
        end)
end

-- Tests: TODO. Add more tests
target("tests")
    set_kind("binary")
    add_files("tests/unit_test.c")
    add_deps("unit_platform_console")

-- Packaging
includes("@builtin/xpack")
xpack("unit")
    set_basename("unit-v$(version)")
    set_formats("zip")
    add_targets("unit_header")
    set_includedir(".")
    add_installfiles(platforms)
