#!/bin/bash
image="zoowii/hxcore_hxbuild:latest"
project_dir=`pwd`
container_id=`docker run -t $image -v $project_dir:/code /bin/bash`
docker exec -t $container_id cmake .
docker exec -t $container_id make -j2

