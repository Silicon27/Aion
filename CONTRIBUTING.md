# Contributing

Well it is a pleasure to have you here! Kaion is always on the board for anyone to scrutinize, including you, my friend. Thanks for considering contributing to Kaion!

## Getting started

Rather than downloading the built executable, contributors would be required to download the raw source folder or clone from the repository. 

If it is your first time using `git`, you can follow the official [Git documentation](https://git-scm.com/book/en/v2/Getting-Started-Installing-Git) to install it on your system. Once installed, you can clone the repository using the following command:

```bash
git clone https://github.com/Silicon27/Aion.git
```

This will create a local copy of the repository on your machine. You can then navigate to the project directory and start making changes to the codebase.

## Guidelines & Conventions

Kaion is rather soft on its restrictions for contributions. There exists a couple of simple, baseline requirements for all contributors:
1. Git acts as the standardized version control system for Kaion
2. Commits should always be descriptive and meaningful, by which changes per-commit should also be kept relatively small (i.e., implementing a simple 20~ line function or fixing a small bug). Larger, or significant modifications/additions shall be severed into multiple commits if applicable.
3. The naming convention for types (i.e., `struct`, `class`, `enum`/`enum class`, `typedef`/`using`, or anything that names or creates a new type) is camel case (i.e., `MyName`, `MyType`, etc), and must have a capital initial letter. 
4. The naming convention for functions, variables, and other identifiers is snake case (i.e., `my_function`, `my_variable`, etc), and must have a lowercase initial letter (if the context allows for the lowercase letter).
5. Doxygen style documentation (specifically with `///`, block documentation comments `/* ... */` are not prefered) is required for all public APIs and functions. Refer to [Doxygen Documentation](https://www.doxygen.nl/manual/docblocks.html) for more information.
6. Public APIs should always be accompanied with its corresponding suite or test. Aion has a custom test framework, refer to [Aion test suite guide](test/suite/aion_test_docs.md) for more information.