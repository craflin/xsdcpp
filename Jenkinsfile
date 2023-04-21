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
                            script {
                                if (isUnix()) {
                                    sh 'rm -rf build/xsdcpp-*.zip'
                                }
                                else {
                                    bat 'if exist build\\xsdcpp-*.zip del build\\xsdcpp-*.zip'
                                }
                                cmakeBuild buildDir: 'build', installation: 'InSearchPath', buildType: 'MinSizeRel', cmakeArgs: '-G Ninja'
                                cmake workingDir: 'build', arguments: '--build . --config MinSizeRel --target package', installation: 'InSearchPath'
                                dir('build') {
                                    archiveArtifacts artifacts: 'xsdcpp-*.zip'
                                }
                            }
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

