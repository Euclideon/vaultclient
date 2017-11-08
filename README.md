# Euclideon Vault Client

This is the for Euclideon Vault Client.

The Euclideon Vault Development team will be monitoring and responding to issues as best we can. Please attempt to avoid filing duplicates of open or closed items when possible.

_We are open-sourcing to increase transparency of our development efforts and to engage the incredibly skilled developers in the open-source and GIS communities._

## Reporting Vulnerabilities

Vulnerabilites should be reported via email to vulnerabilities@euclideon.com [[PGP Key]](https://www.euclideon.com/vulnerabilities-pgp).

## Getting Started

You will *need* Euclideon Vault Development Kit (VDK) you can [sign up here](https://zfrmz.com/EmzYcU5zn7axokCDMfuW).

1. Check out the entire repo (note there is a chain of submodules that you need)
2. Generate project files
  a) For Windows + VS2019, run `create_project.bat` and follow the prompts
  b) Other platform will be provided soon... (looking at `buildscripts/build.sh` will point you in the right direction though)
  
From that you can build and run in your preferred environment.

## Contributing

You've created a new fix or feature for Euclideon Vault Client, awesome!

1. If you haven't already, create a fork of the repository
2. Create a topic branch, and make all of your changes on that branch
3. Submit a pull request
4. We'll get to it as fast as we can.

### Writing a Good Pull Request

- Stay focused on a single fix or feature. If you submit multiple changes in a single request, we may like some but spot issues with others. When that happens, we have to reject the whole thing. If you submit each change in its own request it is easier for us to review and approve.
- Limit your changes to only what is required to implement the fix or feature. In particular, avoid style or formatting tools that may modify the formatting of other areas of the code. If your code editor supports [EditorConfig](https://editorconfig.org), turn it on to use the included editorconfig script.
- When you submit a change, try to limit the number of commits involved.
- Follow our coding conventions in [this document](./coding-standard.md).

## Labels
We will be tagging issues and pull requests in the following way:

- **bug** – We consider this issue to be a bug. This tag is for bugs in implemented features, or something we consider to be a “bug level” change. Things marked with Bug have a corresponding bug in the Euclideon internal bug tracking system.
  - Example: A specific LAS file fails to convert correctly

- **feature** – Denotes something that is not yet implemented.  
  - Example: Exporting Camera flythrough's to Video files

- **discussion** – Denotes a discussion on the board that does not relate to a specific feature.
  - Example: Vault Client's loading screen should display landmarks from around the world

- **fixed** – When possible, we will mark issues that have been completed internally. Unfortunately we cannot say specifically when the next release that includes the change will be.

- **wontfix** – Denotes an issue we won't be making a fix for.  We will give some reasoning why in resolving, e.g. it could be working by design as we see it. After one week we will either close the issue or mark as **discussion** depending on what comes up.

- **buildpending** - The pull requests requires a Euclideon developer to manually test or build the branch
  - Example: If a pull request makes changes to the core rendering module, testing on other platforms will be required before the PR can be merged

- **vdkupdaterequired** - The issue or pull request cannot be resolved without updates to VDK

- **good first issue** - This doesn't require in depth knowledge of the system and is good for newcomers

- **help wanted** - Another set of eyes would be good to look over the details
  - Example: If a PR is complicated or touches multiple systems, another developer to verify the PR helps to reduce errors

Additional tags may be used to denote specific types of issues or components of the system.
  - Example: **rendering** or **inputs**
  
## Important Links
- The Euclideon Vault [Homepage](https://www.euclideon.com/vault/)