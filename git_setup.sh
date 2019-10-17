# This file checks if git is correctly setup
# If not, it enters dummy details so that commits can be made for patching

if ! git config --list | grep -q user.email ; then
  echo "Adding default git config..."
  git config --global user.email "example@example.com"
  git config --global user.name "Example"
fi