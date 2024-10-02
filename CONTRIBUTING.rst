Contribution guidelines
=======================

Contributions to the Acts project are very welcome and feedback on the
documentation is greatly appreciated.

Mailing lists
-------------

-  `acts-users@cern.ch <https://e-groups.cern.ch/e-groups/Egroup.do?egroupName=acts-users>`_:
   Users of the Acts project should subscribe to this list as it provides

   -  regular updates on the software,
   -  a common place for asking any kind of questions.

-  `acts-developers@cern.ch <https://e-groups.cern.ch/e-groups/Egroup.do?egroupName=acts-developers>`_:
   For discussions about

   -  information about developer meetings,
   -  a common place for technical discussions.

Bug reports and feature requests
--------------------------------

To report an issue and before starting work, please create an issue in the
`GitHub issue tracker <https://github.com/acts-project/acts/issues>`_. A
comprehensive explanation will help the development team to respond in a timely
manner. Verbalising the issue before starting work allows the other contributors
to chime in and avoids disagreements how to progress.

-  The title should summarise the issue
-  Describe the issue in as much detail as possible in the comment

GitHub does not allow editing labels, assignees or setting milestone to
non-members of a project with at least "Triage" permission. These will have to
be set by members with Triage permission after an issue/PR is created.
Guidelines regarding labels, assignees and milestone therefore only concern
members of acts-project with the necessary rights and can be ignored by others.

-  Assign to yourself or leave empty
-  Choose labels as appropriate

   -  Type of issue
   -  Which component is affected
   -  Urgency
   -  Fix versions

-  Bug reports

   -  Mention affected version(s)
   -  Issue type: ``Bug``
   -  A detailed description of the bug including a recipe on how to
      reproduce it and any hints which may help diagnosing the problem

-  Feature requests

   -  Issue type: ``Improvement`` or ``Feature``
   -  A detailed description of the feature request including possible
      use cases and benefits for other users

Make a contribution
-------------------

Anyone is welcome to contribute to Acts. Below is a short description how to
contribute. If you have any questions, feel free to ask `acts-developers@cern
<mailto:acts-developers@cern.ch>`_ for help or guidance.

Please always fork the Acts repository you want to work on and create branches
only in your own fork. Once you want to share your work, create a Pull Request
(PR) (for gitlab users: equivalent to merge request) to the main branch of the
upstream acts-project repository. If it is not yet ready to be merged in, please
create a draft pull request (by clicking on the small arrow on the green "create
pull request" button) to mark it work in progress. Once you want your branch to
be merged in, request a review from the `reviewers team
<https://github.com/orgs/acts-project/teams/reviewers>`_. Once a draft merge
request is reviewed, it can be merged in.

To get started with git, please refer to the `short introduction
<https://git-scm.com/docs/gittutorial>`_ as well as the `full git documentation
<https://git-scm.com/doc>`_. Tutorials as well as explanations of concepts and
workflows with git can also be found on `Atlassian
<https://www.atlassian.com/git/>`_.

Checklist for pull requests
~~~~~~~~~~~~~~~~~~~~~~~~~~~

-  Your branch has been rebased on the target branch and can be
   integrated through a fast-forward merge.
-  A detailed description of the pull request is provided.
-  The issue the PR closes is linked.
-  All CI jobs pass.
-  All newly introduced functions and classes have been documented
   properly with doxygen.
-  Unit tests are provided for new functionalities.
-  For bugfixes: a test case has been added to avoid the re-appearance
   of this bug in the future.

Workflow recommendations
~~~~~~~~~~~~~~~~~~~~~~~~

In the following a few recommendations are outlined which should help you to get
familiar with development process in the Acts project.

#. **Each development in its own branch of your private fork!**

   Write access for developers has been disabled for developers on
   acts-project. Therefore, always start by creating your own fork and
   creating branches there. You should start a new branch for every
   development and all work which is logically/conceptually linked
   should happen in one branch. Try to keep your branches short. This
   helps immensely to understand the git history if you need to look at
   it in the future and helps reviewers of your code. If projects are
   complex (e.g. large code refactoring or complex new features), you
   may want to use *sub*-branches from the main development branch.

#. **Never, ever directly work on any "official" branch!**

   Though not strictly necessary and in the end it is up to you, it is strongly
   recommended that you never commit directly on a branch which tracks
   an "official" branch. As all branches are equal in git, the
   definition of "official" branch is quite subjective. In the Acts
   project you should not work directly on branches which are
   **protected** in the repository. Usually, these are the *main* and
   *release/X.Y.Z* branches. The benefit of this strategy is that you
   will never have problems to update your fork. Any git merge in your
   local repository on such an "official" branch will always be a
   fast-forward merge.

#. **Use atomic commits!**

   Similarly to the concept of branches, each
   commit should reflect a self-contained change. Try to avoid overly
   large commits (bad examples are for instance mixing logical change
   with code cleanup and typo fixes).

#. **Write good commit messages!**

   Well-written commit messages are key
   to understand your changes. There are many guidelines available on
   how to write proper commit logs (e.g.
   `here <https://alistapart.com/article/the-art-of-the-commit>`__,
   `here <https://cbea.ms/git-commit/>`__, or
   `here <https://wiki.openstack.org/wiki/GitCommitMessages#Information_in_commit_messages>`__).
   As a short summary:

   -  Structure your commit messages into short title (max 50
      characters) and longer description (max width 72 characters)! This
      is best achieved by avoiding the ``commit -m`` option. Instead
      write the commit message in an editor/git tool/IDE...
   -  Describe why you did the change (git diff already tells you what
      has changed)!
   -  Mention any side effects/implications/consequences!

#. **Prefer git pull --rebase!**

   If you work with a colleague on a new
   development, you may want to include their latest changes. This is
   usually done by calling ``git pull`` which will synchronise your
   local working copy with the remote repository (which may have been
   updated by your colleague). By default, this action creates a merge
   commit if you have local commits which were not yet published to the
   remote repository. These merge commits are considered to contribute
   little information to the development process of the feature and they
   clutter the history (read more e.g.
   `here <https://www.atlassian.com/blog/it-teams/stop-foxtrots-now>`__
   or
   `here <https://fangpenlin.com/posts/2013/09/30/keep-a-readable-git-history/>`__).
   This problem can be avoided by using ``git pull --rebase`` which
   replays your local (un-pushed) commits on the tip of the remote
   branch. You can make this the default behaviour by running
   ``git config pull.rebase true``. More about merging vs rebasing can
   be found
   `here <https://www.atlassian.com/git/tutorials/merging-vs-rebasing/>`__.

#. **Update the documentation!**

   Make sure that the documentation is
   still valid after your changes. Perform updates where needed and
   ensure integrity between the code and its documentation.

Coding style and guidelines
~~~~~~~~~~~~~~~~~~~~~~~~~~~

The Acts project uses
`clang-format <https://clang.llvm.org/docs/ClangFormat.html>`_ for
formatting its source code. A ``.clang-format`` configuration file comes
with the project and should be used to automatically format the code.
Developers can use the provided Docker image to format their project or
install clang-format locally. Developers should be aware that
clang-format will behave differently for different versions, so
installing `the same clang version as used in the
CI is recommended. There are several instructions available on how to
integrate clang-format with your favourite IDE (e.g. `Xcode <https://github.com/travisjeffery/ClangFormat-Xcode>`_,
`emacs <https://clang.llvm.org/docs/ClangFormat.html#emacs-integration>`_).
The Acts CI system will automatically check code formatting using the
provided clang-format configuration and will notify incompatible formatting.

In addition, some conventions are used in Acts code, details can be
found `here <https://acts.readthedocs.io/en/latest/codeguide.html>`_.
For Doxygen documentation, please follow these recommendations:

-  Put all documentation in the header files.
-  Use ``///`` as block comment (instead of ``/* ... */``).
-  Doxygen documentation goes in front of the documented entity (class,
   function, (member) variable).
-  Use the ``@<cmd>`` notation.
-  Document all (template) parameters using @(t)param and explain the
   return value for non-void functions. Mention important conditions
   which may affect the return value.
-  Use ``@remark`` to specify pre-conditions.
-  Use ``@note`` to provide additional information.
-  Link other related entities (e.g. functions) using ``@sa``.

Example: Make a bugfix while working on a feature
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

During the development of a new feature you discover a bug which needs
to be fixed. In order to not mix bugfix and feature development, the
bugfix should happen in a different branch. The recommended procedure
for handling this situation is the following:

#. Get into a clean state of your working directory on your feature
   branch (either by committing open changes or by stashing them).
#. Checkout the branch the bugfix should be merged into (either *main*
   or *release/X.Y.Z*) and get the most recent version.
#. Create a new branch for the bugfix.
#. Fix the bug, write a test, update documentation etc.
#. Open a pull request for the bug fix.
#. Switch back to your feature branch.
#. Merge your local bugfix branch into the feature branch. Continue your
   feature development.
#. Eventually, the bugfix will be merged into *main*. Then, you can
   rebase your feature branch on main which will remove all duplicate
   commits related to the bugfix.

Example: Backporting a feature or bugfix
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Suppose you have a bugfix or feature branch that is eventually going to
be merged in ``main``. You might want to have the feature/bugfix
available in a patch (say ``0.25.1``) tag. To to that, find the
corresponding release branch, for this example that would be
``release/v0.25.X``. You must create a dedicated branch that **only**
contains the commits that relate to your feature/bugfix, otherwise the
PR will contain all other commits that were merged into main since the
release was branched off. With that branch, open a PR to that branch,
and make it clear that it is a backport, also linking to a potential
equivalent PR that targets ``main``.

Tips for users migrating from GitLab
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

-  The most obvious difference first: What is called Merge Request
   in GitLab is called Pull Request (PR) in GitHub.
-  Once your PR is ready to be merged, request a review by the users in
   the `reviewers
   team <https://github.com/orgs/acts-project/teams/reviewers>`__
-  As Acts started enforcing using your own fork with the switch to
   GitHub, developers no longer have write access to the upstream
   repository.
-  The CI will fail if a PR does not yet have the required approvals.

Review other contributions
--------------------------

Acts requires that every pull request receives at least one review by a
member of the reviewers team before being merged but anyone is welcome
to contribute by commenting on code changes. You can help reviewing
proposed contributions by going to `the "pull requests" section of the
Acts (core) GitHub
repository <https://github.com/acts-project/acts/pulls>`_.

As some of the guidelines recommended here require rights granted to the
reviewers team, this guide specifically addresses the people in this
team. The present contribution guide should serve as a good indication
of what we expect from code submissions.

Approving a pull request
~~~~~~~~~~~~~~~~~~~~~~~~

-  Does its title and description reflect its contents?
-  Do the automated continuous integration tests pass without problems?
-  Have all the comments raised by previous reviewers been addressed?

If you are confident that a pull request is ready for integration,
please make it known by clicking the "Approve pull request" button of
the GitHub interface. This notifies other members of the Acts team of
your decision, and marks the pull request as ready to be merged.

Merging a pull request
~~~~~~~~~~~~~~~~~~~~~~

If you have been granted write access on the Acts repository, you can
merge a pull request into the Acts main branch after it has been
approved.

GitHub may warn you that a "Fast-forward merge is not possible". This
warning means that the pull request has fallen behind the current Acts
main branch, and should be updated through a rebase. Please notify the
pull request author in order to make sure that the latest main changes
do not affect the pull request, and to have it updated as appropriate.

For a PR that is behind main, a button "Update branch" may appear.
This should NOT be used as it merges instead of rebasing, which is not
our workflow.
