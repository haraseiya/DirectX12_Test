#include <vector>
#include <functional>
#include <iostream>

using namespace std;

int main()
{
	// コマンドリストを模したもの
	std::vector<std::function<void(void)>> commandlist;

	// 命令１
	commandlist.push_back([]() {cout << "GPU Set RTV-1" << endl; });

	cout << "CPU Set命令-2" << endl;

	// 命令２
	commandlist.push_back([]() {cout << "GPU Clear RTV-3" << endl; });

	cout << "CPU Clear命令-4" << endl;

	// 命令３
	commandlist.push_back([]() {cout << "GPU Close-5" << endl; });

	cout << "CPU Close命令-6" << endl;

	cout << endl;

	// コマンドキューのExecuteCommandを模した処理
	for (auto& cmd : commandlist)
	{
		cmd();
	}

	getchar();
	return 0;
}