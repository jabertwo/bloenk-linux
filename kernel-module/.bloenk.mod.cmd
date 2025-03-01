savedcmd_bloenk.mod := printf '%s\n'   bloenk.o | awk '!x[$$0]++ { print("./"$$0) }' > bloenk.mod
