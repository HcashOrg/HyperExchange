#!/bin/bash
image="zoowii/hxcore_hxbuild:latest"
project_dir=`pwd`
container_name="hxcoredev-ci"
docker run -t $image -v $project_dir:/code --name $container_name /bin/bash
echo $container_name
docker exec $container_name cmake .
docker exec $container_name ./build_node.sh

