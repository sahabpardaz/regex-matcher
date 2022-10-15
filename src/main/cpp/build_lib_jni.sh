#!/usr/bin/env bash

function main() {
  script_dir=$( cd "$(dirname "${BASH_SOURCE[0]}")" ; pwd -P )
  regex_matcher_maven_resources_path=${script_dir}/../resources
  regex_matcher_docker_path=${script_dir}/Dockerfile

  echo "Building Chimera JNI library ..."
  sudo docker build -t lib_jni_builder:build --network=host . -f ${regex_matcher_docker_path}
  sudo docker container create --name lib_jni lib_jni_builder:build

  echo "Copy Chimera JNI library file to maven resources path ..."
  sudo docker container cp lib_jni:/tmp/build-files/build/jni/libchimera_jni.so \
    "${regex_matcher_maven_resources_path}/lib_jni.so"

  echo "Compress JNI library file ..."
  cd "${regex_matcher_maven_resources_path}"
  tar -czf lib_jni.tar.gz lib_jni.so
  rm lib_jni.so

  echo "Remove created containers ..."
  sudo docker container rm -f lib_jni

  echo "Build successful. You should build your project again."
}

set -euo pipefail  # Fail on errors and unset vars.
main $@
