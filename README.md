Brand new pintos for Operating Systems and Lab (CS330), KAIST, by Youngjin Kwon.

The manual is available at https://casys-kaist.github.io/pintos-kaist/.

💡 AWS EC2 Ubuntu 22.04 (x86_64)에서 진행합니다.

1) EC2를 다음과 같이 세팅합니다. (한 줄씩 입력)
  $ sudo apt update
  $ sudo apt install -y gcc make qemu-system-x86 python3
2) pintos repository 복제
아래와 같이 git 명령을 사용하여 GitHub에 있는 과제를 다운로드 하고 자기 팀의 git repository로 옮깁니다. 원본 repository에 있는 코드를 개선하기 위해 clone 혹은 fork하는 것이 아니고 팀의 repository로 복제(duplicate) 하는 것임을 명심하십시오.

참고: GitHub: Duplicating a repository
  $ git clone --bare https://github.com/SWJungle/pintos-kaist.git
  $ cd pintos-kaist.git
  $ git push --mirror https://github.com/${교육생 or 팀ID}/pintos-kaist.git
  $ cd ..
  $ rm -rf pintos-kaist.git
  $ git clone https://github.com/${교육생ID}/pintos-kaist.git
팀 repository는 GitHub이 아니라 Bitbucket, Gitlab 등 다른 git repository여도 됩니다. 자신과 팀의 작업을 계속 기록하고 남겨두어서 작업방식의 변경, 개발 환경의 장애 발생 등에 대응할 수 있으면 됩니다.

3) PintOS가 제대로 동작되는지 확인 (Project 1의 경우)
  $ cd pintos-kaist
  $ source ./activate
  $ cd threads
  $ make check
  # 뭔가 한참 compile하고 test 프로그램이 돈 후에 다음 message가 나오면 정상
  20 of 27 tests failed.
