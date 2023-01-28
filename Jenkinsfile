pipeline {
    agent none
    stages {
        stage('All') {
            matrix {
                agent {
                    label "${platform}"
                }
                axes {
                    axis {
                        name 'platform'
                        values 'ubuntu22.04-x86_64','ubuntu20.04-x86_64', 'ubuntu18.04-x86_64', 'windows10-x64', 'windows10-x86', 'rocky8-x86_64'
                    }
                }
                stages {
                    stage('Build') {
                        steps {
                            cmakeBuild buildDir: 'build', installation: 'InSearchPath', buildType: 'MinSizeRel', cmakeArgs: '-G Ninja'
                            cmake workingDir: 'build', arguments: '--build . --target package', installation: 'InSearchPath'
                        }
                    }
                    stage('Test') {
                        steps {
                            ctest workingDir: 'build', installation: 'InSearchPath', arguments: '--output-on-failure'
                        }
                    }
                }
            }
        }
    }
}

