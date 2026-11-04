So the idea here is to use to create a mask for nvim AND VIM, so that it behaves similarly to zoxide and atuin. So this is a CPP based mask, basically. That keeps track of the recent program by storing what the inputs and create some statistics and usage in a similar fashion way to the previously mentioned programs. And the idea is to create an easier access to programs using nvim

To sue the functionality of nvim2 a line in .bashrc has to be added
    `eval($"nvim --init bash")`
So that by pressinbg a key combination this can make autocompletion possible


The history is located in `.local/share/nvim2`

