
import("core.base.task")
import("core.project.project")
import("lib.detect.find_program")


function _end_with_error(description)
    cprint("${red}error:${clear} " .. description)
    os.exit(-1)
end

-- I (the creator of this library) use jj instead of git when it is possible.
-- jj likes to work in branchless mode and it doesn't support tags, so we need
-- this extra step to "transition to git", so we can create the tag for the
-- release
function _transition_from_jj(git)
    if not os.isdir(".jj") then
        return
    end
    local jj = find_program("jj")
    if not jj then
        _end_with_error(".jj directory detected but jj binary not found")
    end

    -- verify the current change is empty
    -- If the current change is not empty, git will detect uncommited changes
    local is_empty, err = os.iorunv(jj, {"log", "-r@", "-T", "empty", "--no-graph", "--color", "never"})
    is_empty = string.trim(is_empty)
    if is_empty ~= "true" then
        _end_with_error("Current change is not empty, run jj new")
    end

    -- verify that the git_head() commit points to main
    local is_main, err2 = os.iorunv(jj, {"log", "-r", "git_head()", "-T", "bookmarks.filter(|s| s.name()=='main').len() == 1", "--no-graph", "--color", "never"})
    is_main = string.trim(is_main)
    if not is_main == "true" then
        _end_with_error("jj git head is not main")
    end

    -- now, we can safely invoke "git checkout main" to transition into a valid
    -- git state
    os.execv(git, {"checkout", "main"})
end


function _git_current_branch(git)
    local branch, err = os.iorunv(git, {"rev-parse", "--abbrev-ref", "HEAD"})
    return string.trim(branch)
end

function _git_uncommited_changes(git)
    local changes, err = os.iorunv(git, {"status", "--porcelain=v1"})
    changes = string.trim(changes)
    print("debug-changes \"" ..changes .. "\"")
    if changes ~= "" then
        return true
    else
        return false
    end
end

function _git_create_tag(git, version)
    os.runv(git, {"tag", "-a", "v"..version, "-m", "Release " .. version})
    os.runv(git, {"push", "origin", "v"..version})
end

function _gh_version_exists(gh, version)
    local result = os.execv(gh, {"release", "view", "v"..version}, {try = true, stdout = os.nuldev(), stderr = os.nuldev()})
    return result == 0
end

function _gh_create_release(gh, version)
    os.runv(gh, {"release", "create",  "v"..version, "build/xpack/unit/unit-v" .. version .. ".zip" })
end

function main()
    -- TODO: run tests and sanity checks
    print("Generating release for version: " .. project.version())

    -- find git
    local git = find_program("git")
    if not git then
        _end_with_error("git not found")
    end

    -- find gh (no idea why this is not working if .exe is not added on windows)
    local gh_name = "gh"
    if os.is_host("windows") then
        gh_name = gh_name .. ".exe"
    end
    local gh = find_program(gh_name)
    if not gh then
        _end_with_error("gh command not found")
    end

    if _gh_version_exists(gh, project.version()) then
        _end_with_error("Version " .. project.version() .. " already exists in Github")
    end

    -- handle the case jj has been used instead of git
    _transition_from_jj(git)

    -- check the current branch
    if _git_current_branch(git) ~= "main" then
        _end_with_error("Current branch is not 'main'")
    end

    -- check there is no uncommited changes
    if _git_uncommited_changes(git) then
        _end_with_error("Uncommited changes")
    end

    print("Running pack")
    task.run("pack")

    -- create and push the tag
    _git_create_tag(git, project.version())

    -- create gh release
    _gh_create_release(gh, project.version())
end
