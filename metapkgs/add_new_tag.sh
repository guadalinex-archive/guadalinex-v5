#!/bin/bash

if [ ! -d $PWD/trunk/gcs ]; then
	echo "Error: You must be on the package's root directory"
	exit
fi

function error()
{

	echo -e "\nError: $*"
	exit
}

PKG=$(basename $PWD)
REVISION=$(svnversion trunk)
TAG_NAME=${PKG}-v5r${REVISION}


# Update the trunk directory to avoid svnversions with ':'
cd trunk
echo -n "Copying trunk into new tag tags/$TAG_NAME..."
svn up -q || error "Trying to update the trunk"
echo "OK"
cd -

# Copy the trunk directory as a new tag
echo -n "Copying trunk into new tag tags/$TAG_NAME..."
svn cp -q trunk tags/$TAG_NAME || error "Trying to copy the trunk"
echo "OK"

# Commit the changes
echo -n "Making commit of the changes..."
svn ci -q -m "Added new tag version $TAG_NAME for the package $PKG" tags/$TAG_NAME || error "Trying to commit the changes"
echo "OK"

