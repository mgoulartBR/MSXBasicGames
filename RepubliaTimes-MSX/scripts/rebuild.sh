#!/bin/bash
source "$(dirname "$0")/env.sh"
"$(dirname "$0")/clean.sh" && "$(dirname "$0")/build.sh"
