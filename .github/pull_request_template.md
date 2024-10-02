PLEASE FOLLOW THE CHECKLIST BELOW WHEN CREATING A NEW PULL REQUEST. THE
CHECKLIST IS FOR YOUR INFORMATION AND MUST BE REMOVED BEFORE SUBMITTING THE PULL
REQUEST.

## Checklist

- [ ] Does the PR title follow the `<prefix>: title` scheme?

    The prefix must be one of:

  - `fix`: for a bugfix
  - `feat`: for a new feature
  - `refactor`: for an improvement of an existing feature
  - `perf`, `test`: for performance- or test-related changes
  - `docs`: for documentation-related changes
  - `build`, `ci`, `chore`: as appropriated for infrastructure changes

- [ ] Does this modify the public API as defined in `docs/versioning.rst`?

  - [ ] Does the PR title contain a `!` to indicate a breaking change?
  - [ ] Is there section starting with `BREAKING CHANGE:` in the PR body
          that explains the breaking change?

- [ ] Is the PR ready to be merged?

  - [ ] If not: is it marked as a draft PR?

- [ ] Does this PR close an existing issue?

  - [ ] Is the issue correctly linked so it will be automatically closed
        upon successful merge (See closing keywords link in the sidebar)?

- The CI will initially report a missing milestone. One of the maintainers will
  handle assigning a milestone for book-keeping.

- An automated workflow will assign labels based on changed files, and whether
  or not reference files were changed. These do not have to be set manually.

- If you push updates, and you know they will be superseded later on, consider adding
  `[skip ci]` in the commit message. This will instruct the CI system not to run any
  jobs on this commit.
