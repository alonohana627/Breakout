# GameLib revision

Breakout and GameLib are separate repositories, and commits across
repositories are not atomic. This file records which GameLib revision each
Breakout change was validated against (build with zero warnings, smoke runs
with the validation layer). `CMakeLists.txt` pins GameLib by full commit hash
(`GIT_TAG`, `GIT_SHALLOW FALSE`), so checking out a Breakout commit reproduces
its combination by itself; CMake prints the revision it builds against
(`Breakout: using GameLib ...`).

| Breakout change | GameLib revision | Validated |
|---|---|---|
| Project from the new-game template (`build`) | `53e8c13` | clean build fetching the commit, 1/1 smoke run |
