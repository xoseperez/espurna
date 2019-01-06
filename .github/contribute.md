Do you want to do a pull request?

First things first: **THANK YOU!**. ESPurna started as a personal project and it will be great if it becomes a community project. There are so many things that can be improved, added and fixed (yeah, a lot a small bugs and not so small bugs there, I'm sure). And sometimes I just don't have the time to work on it as much as I'd like to. 

Second. Let's try to keep it homogeneous and readable. I have my coding style. It's mostly standard but sometimes it can be opinionated. It you are willing to do a pull request, there are a few things I would ask you first:

## Pull request ##
* Do the pull request against the **`dev` branch**
* **Only touch relevant files** (beware if your editor has auto-formatting feature enabled)
* If you are adding a new functionality (new hardware, new library support) not related to an existing component move it to it's **own modules** (.ino file)
* If you are adding new library, include it in one of the **sample travis profiles**, so our integrated CI will try to compile it.
* Make sure you check [Coding Style](https://github.com/xoseperez/espurna/wiki/CodingStyle)
* PRs that don't compile (break Travis) or cause more coding errors (as reported by Codacy) will not be merged. Please fix the issue. Same goes for PRs that are raised against older commit in dev - you might need to rebase and resolve conflicts.


And thank you again!
