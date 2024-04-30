#!/bin/bash

# run troyDocker.sh to run the image as a container in the code working directory, which is linked to the host files.
docker run --rm -it -v "$(pwd):/code" -w /code discordwrapped /bin/bash