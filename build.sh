#!/bin/bash
image="zoowii/hxcore_hxbuild:latest"
project_dir=`pwd`
container_name="hxcoredevci"
docker run -t -d -v $project_dir:/code --name $container_name $image /bin/bash
echo $container_name
docker exec $container_name git submodule update --init --recursive
docker exec $container_name cmake .
docker exec $container_name ./build_node.sh

