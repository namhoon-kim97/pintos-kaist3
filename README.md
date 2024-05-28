Brand new pintos for Operating Systems and Lab (CS330), KAIST, by Youngjin Kwon.

The manual is available at https://casys-kaist.github.io/pintos-kaist/.

💡 AWS EC2 Ubuntu 22.04 (x86_64)에서 진행합니다.

매뉴얼: [Pintos-KAIST 매뉴얼](https://casys-kaist.github.io/pintos-kaist/)

💡 AWS EC2 Ubuntu 22.04 (x86_64) 설정 지침

## 1단계: 필수 패키지 업데이트 및 설치
sudo apt update<br>
sudo apt install -y gcc make qemu-system-x86 python3<br>
## 2단계: Pintos 저장소 복제
다음 git 명령을 사용하여 GitHub에서 과제를 다운로드하고 팀의 git 저장소로 이동하십시오. 원본 저장소를 개선하기 위해 클론 또는 포크하는 것이 아니라 팀의 저장소로 복제하는 것입니다.

git clone --bare https://github.com/SWJungle/pintos-kaist.git<br>
cd pintos-kaist.git<br>
git push --mirror https://github.com/${교육생 or 팀ID}/pintos-kaist.git<br>
cd ..<br>
rm -rf pintos-kaist.git<br>
git clone https://github.com/${교육생ID}/pintos-kaist.git<br>
참고: 팀 저장소는 GitHub, Bitbucket, Gitlab 또는 기타 git 저장소 서비스일 수 있습니다. 작업 방식의 변경이나 개발 환경의 장애 발생 시 대응할 수 있도록 자신과 팀의 작업을 계속 기록하고 남겨 두는 것이 중요합니다.

## 3단계: Pintos 설정 확인 (프로젝트 1의 경우)
다음 명령어를 실행하여 Pintos가 제대로 작동하는지 확인하십시오:

cd pintos-kaist<br>
source ./activate<br>
cd threads<br>
make check<br>

예상 출력:
20 of 27 tests failed.
