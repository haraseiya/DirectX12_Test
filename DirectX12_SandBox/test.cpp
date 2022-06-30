#include <vector>
#include <functional>
#include <iostream>

using namespace std;

int main()
{
	// �R�}���h���X�g��͂�������
	std::vector<std::function<void(void)>> commandlist;

	// ���߂P
	commandlist.push_back([]() {cout << "GPU Set RTV-1" << endl; });

	cout << "CPU Set����-2" << endl;

	// ���߂Q
	commandlist.push_back([]() {cout << "GPU Clear RTV-3" << endl; });

	cout << "CPU Clear����-4" << endl;

	// ���߂R
	commandlist.push_back([]() {cout << "GPU Close-5" << endl; });

	cout << "CPU Close����-6" << endl;

	cout << endl;

	// �R�}���h�L���[��ExecuteCommand��͂�������
	for (auto& cmd : commandlist)
	{
		cmd();
	}

	getchar();
	return 0;
}