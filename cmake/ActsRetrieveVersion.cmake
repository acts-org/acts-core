# Retrieve version identification.
#
# Must be included from the main CMakeLists file. Sets the following variables:
#
#   - _acts_version
#   - _acts_commit_hash
#

# read version number from file
file(READ "version_number" _acts_version)
string(STRIP "${_acts_version}" _acts_version)

# read commit hash from git
include(GetGitRevisionDescription)
get_git_head_revision(_git_refspec _git_hash)
string(SUBSTRING "${_git_hash}" 0 9 _git_hash_short)
git_local_changes(_git_local_changes)
if(_git_local_changes STREQUAL "CLEAN")
    set(_acts_commit_hash "${_git_hash}")
    set(_acts_commit_hash_short "${_git_hash_short}")
else()
    set(_acts_commit_hash "${_git_hash}-dirty")
    set(_acts_commit_hash_short "${_git_hash_short}-dirty")
endif()

# remove temporary variables
unset(_git_refspec)
unset(_git_hash)
unset(_git_hash_short)
unset(_git_local_changes)
