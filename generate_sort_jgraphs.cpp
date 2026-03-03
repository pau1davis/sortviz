//sortviz source code
//Paul Davis : pdavis51
#include <algorithm>  // sort stuff
#include <chrono>  // timestamp generation 
#include <cstdlib>  // system() calls for shell command execution
#include <filesystem>  // path handling
#include <fstream>  
#include <functional>  // sort stuff
#include <iomanip>  // formatting
#include <iostream>  
#include <sstream>  
#include <stdexcept>  // runtime_error exceptions for validation failures
#include <string> 
#include <vector>  

using namespace std;

struct SortStep {
	int step;
	vector<int> values;
	string note;
	vector<int> compareIndices;
	vector<int> moveFromIndices;
	vector<int> moveToIndices;
};

// returns true when a highlight index list contains the requested index
bool containsIndex(const vector<int>& indices, int value) {
	return find(indices.begin(), indices.end(), value) != indices.end();
}

// splits a comma-delimited string into non-empty tokens
vector<string> splitComma(const string& text) {
	vector<string> tokens;
	stringstream stream(text);
	string part;
	while (getline(stream, part, ',')) {
		if (!part.empty()) {
			tokens.push_back(part);
		}
	}
	return tokens;
}

// escapes a string for single quoted use in shell commands
string shellQuote(const string& value) {
	string quoted = "'";
	for (char ch : value) {
		if (ch == '\'') {
			quoted += "'\\''";
		} else {
			quoted += ch;
		}
	}
	quoted += "'";
	return quoted;
}

// checks whether a command is available on PATH
bool commandExists(const string& name) {
	string check = "command -v " + name + " >/dev/null 2>&1";
	return system(check.c_str()) == 0;
}

// resolves which jgraph executable to use
string resolveJgraphCommand() {
	vector<string> candidates;

	if (const char* envJgraph = getenv("JGRAPH")) {
		string value(envJgraph);
		if (!value.empty()) {
			candidates.push_back(value);
		}
	}

	candidates.push_back("/home/jplank/bin/LINUX-X86_64/jgraph");
	if (filesystem::exists("./jgraph")) {
		candidates.push_back("./jgraph");
	}
	candidates.push_back("jgraph");

	for (const auto& candidate : candidates) {
		if (commandExists(candidate)) {
			return candidate;
		}
	}

	return "";
}

// parses command line data from space-separated and/or comma-separated args
vector<int> parseValues(int argc, char* argv[]) {
	if (argc < 4) {
		throw runtime_error(
				"Provide values after sort type. Example: bubble 5 1 4 2 8 or "
				"bubble 5,1,4,2,8");
	}

	vector<int> values;
	for (int i = 2; i < argc; i++) {
		string arg = argv[i];
		auto commaParts = splitComma(arg);
		if (!commaParts.empty()) {
			for (const auto& token : commaParts) {
				values.push_back(stoi(token));
			}
		} else {
			values.push_back(stoi(arg));
		}
	}

	if (values.empty()) {
		throw runtime_error("No numeric values were parsed.");
	}

	return values;
}

// runs bubble sort while recording a visualization step after each comparison
vector<SortStep> bubbleSortSteps(const vector<int>& input) {
	vector<int> arr = input;
	vector<SortStep> steps;
	steps.push_back({0, arr, "initial", {}, {}, {}});

	int stepIndex = 1;
	for (int end = static_cast<int>(arr.size()) - 1; end > 0; end--) {
		for (int i = 0; i < end; i++) {
			int j = i + 1;
			if (arr[i] > arr[j]) {
				swap(arr[i], arr[j]);
				steps.push_back(
						{stepIndex++,
						arr,
						"compare (" + to_string(i) + "," + to_string(j) + ") swap",
						{i, j},
						{i, j},
						{i, j}});
			} else {
				steps.push_back({stepIndex++,
						arr,
						"compare (" + to_string(i) + "," +
						to_string(j) + ") no-swap",
						{i, j},
						{},
						{}});
			}
		}
	}

	vector<int> sorted = input;
	sort(sorted.begin(), sorted.end());
	if (steps.back().values != sorted) {
		steps.push_back({stepIndex, arr, "sorted", {}, {}, {}});
	}

	return steps;
}

// runs merge sort while recording write operations for each merge action
vector<SortStep> mergeSortSteps(const vector<int>& input) {
	vector<int> arr = input;
	vector<int> aux = arr;
	vector<SortStep> steps;
	steps.push_back({0, arr, "initial", {}, {}, {}});
	int stepIndex = 1;

	function<void(int, int)> sortRange = [&](int lo, int hi) {
		if (lo >= hi) {
			return;
		}

		int mid = (lo + hi) / 2;
		sortRange(lo, mid);
		sortRange(mid + 1, hi);

		for (int i = lo; i <= hi; i++) {
			aux[i] = arr[i];
		}

		int left = lo;
		int right = mid + 1;
		int write = lo;

		while (left <= mid && right <= hi) {
			int fromIndex;
			if (aux[left] <= aux[right]) {
				arr[write] = aux[left];
				fromIndex = left;
				left++;
			} else {
				arr[write] = aux[right];
				fromIndex = right;
				right++;
			}

			steps.push_back({stepIndex++,
					arr,
					"merge [" + to_string(lo) + ":" + to_string(hi) +
					"] write " + to_string(write) + " from " +
					to_string(fromIndex),
					{},
					{fromIndex},
					{write}});
			write++;
		}

		while (left <= mid) {
			arr[write] = aux[left];
			steps.push_back({stepIndex++,
					arr,
					"merge [" + to_string(lo) + ":" + to_string(hi) +
					"] write " + to_string(write) + " from " +
					to_string(left),
					{},
					{left},
					{write}});
			left++;
			write++;
		}

		while (right <= hi) {
			arr[write] = aux[right];
			steps.push_back({stepIndex++,
					arr,
					"merge [" + to_string(lo) + ":" + to_string(hi) +
					"] write " + to_string(write) + " from " +
					to_string(right),
					{},
					{right},
					{write}});
			right++;
			write++;
		}
	};

	sortRange(0, static_cast<int>(arr.size()) - 1);

	vector<int> sorted = input;
	sort(sorted.begin(), sorted.end());
	if (steps.back().values != sorted) {
		steps.push_back({stepIndex, arr, "sorted", {}, {}, {}});
	}

	return steps;
}

// runs quick sort while recording partition comparisons and swaps
vector<SortStep> quickSortSteps(const vector<int>& input) {
	vector<int> arr = input;
	vector<SortStep> steps;
	steps.push_back({0, arr, "initial", {}, {}, {}});
	int stepIndex = 1;

	function<void(int, int)> quickSortRange = [&](int lo, int hi) {
		if (lo >= hi) {
			return;
		}

		int pivotValue = arr[hi];
		int i = lo - 1;

		for (int j = lo; j < hi; j++) {
			if (arr[j] <= pivotValue) {
				i++;
				if (i != j) {
					swap(arr[i], arr[j]);
					steps.push_back({stepIndex++,
							arr,
							"partition [" + to_string(lo) + ":" +
							to_string(hi) + "] pivot " +
							to_string(pivotValue) + " swap " +
							to_string(i) + "<->" + to_string(j),
							{j, hi},
							{i, j},
							{i, j}});
				} else {
					steps.push_back({stepIndex++,
							arr,
							"partition [" + to_string(lo) + ":" +
							to_string(hi) + "] pivot " +
							to_string(pivotValue) + " keep " +
							to_string(j),
							{j, hi},
							{},
							{}});
				}
			} else {
				steps.push_back({stepIndex++,
						arr,
						"partition [" + to_string(lo) + ":" +
						to_string(hi) + "] pivot " +
						to_string(pivotValue) + " no-swap " +
						to_string(j),
						{j, hi},
						{},
						{}});
			}
		}

		int pivotTarget = i + 1;
		if (pivotTarget != hi) {
			swap(arr[pivotTarget], arr[hi]);
			steps.push_back(
					{stepIndex++,
					arr,
					"pivot swap " + to_string(pivotTarget) + "<->" + to_string(hi),
					{pivotTarget, hi},
					{pivotTarget, hi},
					{pivotTarget, hi}});
		} else {
			steps.push_back({stepIndex++,
					arr,
					"pivot fixed at " + to_string(hi),
					{hi},
					{},
					{}});
		}

		quickSortRange(lo, pivotTarget - 1);
		quickSortRange(pivotTarget + 1, hi);
	};

	quickSortRange(0, static_cast<int>(arr.size()) - 1);

	vector<int> sorted = input;
	sort(sorted.begin(), sorted.end());
	if (steps.back().values != sorted) {
		steps.push_back({stepIndex, arr, "sorted", {}, {}, {}});
	}

	return steps;
}

// renders a single sorting step into jgraph source text
string toJgraph(const SortStep& step, const string& sortType) {
	int n = static_cast<int>(step.values.size());
	int xMax = max(1, n);

	const double cellWidth = 0.8;
	const double graphWidth = min(7.0, max(2.5, xMax * cellWidth));
	const double graphHeight = 2.8;

	ostringstream out;
	out << "(* Auto-generated sort step graph *)\n\n";
	out << "newgraph\n";
	out << "xaxis min 0 max " << xMax << " size " << graphWidth << " nodraw\n";
	out << "yaxis min 0 max 3 size " << graphHeight << " nodraw\n";
	out << "newstring hjc x " << fixed << setprecision(3) << (xMax / 2.0)
		<< " y 2.8 fontsize 12 font Helvetica-Bold : " << sortType << " step "
		<< step.step << " - " << step.note << "\n";

	const double legendX = 0.0;
	const double legendSwatchW = 0.22;
	const double legendSwatchH = 0.12;
	const double legendTextX = legendX + 0.30;

	auto writeLegendRow = [&](double y, double r, double g, double b,
			const string& label) {
		out << "newline poly pcfill " << r << " " << g << " " << b
			<< " color 0 0 0\n";
		out << "  pts " << legendX << " " << y << "\n";
		out << "      " << (legendX + legendSwatchW) << " " << y << "\n";
		out << "      " << (legendX + legendSwatchW) << " "
			<< (y + legendSwatchH) << "\n";
		out << "      " << legendX << " " << (y + legendSwatchH) << "\n";
		out << "newstring hjl vjb x " << legendTextX << " y " << y
			<< " fontsize 9 font Helvetica : " << label << "\n";
	};

	writeLegendRow(2.56, 1.0, 1.0, 0.6, "Compared");
	writeLegendRow(2.38, 1.0, 0.75, 0.85, "Moving from");
	writeLegendRow(2.20, 0.75, 1.0, 0.7, "Moving to");

	for (int i = 0; i < n; i++) {
		double x0 = static_cast<double>(i);
		double x1 = static_cast<double>(i + 1);
		double xm = (x0 + x1) / 2.0;

		double fillR = 1.0;
		double fillG = 1.0;
		double fillB = 1.0;

		if (containsIndex(step.compareIndices, i)) {
			fillR = 1.0;
			fillG = 1.0;
			fillB = 0.6;
		}
		if (containsIndex(step.moveFromIndices, i)) {
			fillR = 1.0;
			fillG = 0.75;
			fillB = 0.85;
		}
		if (containsIndex(step.moveToIndices, i)) {
			fillR = 0.75;
			fillG = 1.0;
			fillB = 0.7;
		}

		out << "newline poly pcfill " << fillR << " " << fillG << " " << fillB
			<< " color 0 0 0\n";
		out << "  pts " << x0 << " 1.0\n";
		out << "      " << x1 << " 1.0\n";
		out << "      " << x1 << " 2.0\n";
		out << "      " << x0 << " 2.0\n";
		out << "newstring hjc vjc x " << xm
			<< " y 1.5 fontsize 14 font Helvetica-Bold : " << step.values[i]
			<< "\n";
		out << "newstring hjc vjt x " << xm
			<< " y 0.85 fontsize 9 font Helvetica : " << i << "\n";
	}

	return out.str();
}

// creates a timestamped output directory for a specific sort run
filesystem::path createRunDir(const string& sortType,
		const filesystem::path& root) {
	auto now = chrono::system_clock::now();
	auto stamp =
		chrono::duration_cast<chrono::seconds>(now.time_since_epoch()).count();

	filesystem::path runDir = root / (sortType + "_" + to_string(stamp));
	filesystem::create_directories(runDir);
	return runDir;
}

// writes all recorded sort steps as numbered .jgr files
int writeSteps(const filesystem::path& runDir, const string& sortType,
		const vector<SortStep>& steps) {
	for (const auto& step : steps) {
		ostringstream fileName;
		fileName << "step_" << setw(4) << setfill('0') << step.step << ".jgr";
		filesystem::path target = runDir / fileName.str();
		ofstream file(target);
		file << toJgraph(step, sortType);
	}
	return static_cast<int>(steps.size());
}

// converts generated .jgr files to .ps, .pdf, and .jpg files
void convertJgraphs(const filesystem::path& runDir) {
	string jgraphCmd = resolveJgraphCommand();

	if (jgraphCmd.empty()) {
		throw runtime_error(
				"jgraph executable not found (checked JGRAPH env var, "
				"/home/jplank/bin/LINUX-X86_64/jgraph, ./jgraph, and PATH)."
		);
	}

	bool hasPs2pdf = commandExists("ps2pdf");
	bool hasPstopdf = commandExists("pstopdf");
	bool hasConvert = commandExists("convert");
	bool hasMagick = commandExists("magick");

	if (!hasPs2pdf && !hasPstopdf) {
		throw runtime_error(
				"Neither ps2pdf nor pstopdf is available to generate PDF files.");
	}

	if (!hasConvert && !hasMagick) {
		throw runtime_error(
				"Neither convert nor magick is available to generate JPG files.");
	}

	vector<filesystem::path> jgrFiles;
	for (const auto& entry : filesystem::directory_iterator(runDir)) {
		if (entry.is_regular_file() && entry.path().extension() == ".jgr") {
			jgrFiles.push_back(entry.path());
		}
	}
	sort(jgrFiles.begin(), jgrFiles.end());

	for (const auto& jgrFile : jgrFiles) {
		filesystem::path psFile = jgrFile;
		psFile.replace_extension(".ps");
		filesystem::path pdfFile = jgrFile;
		pdfFile.replace_extension(".pdf");
		filesystem::path jpgFile = jgrFile;
		jpgFile.replace_extension(".jpg");

		string renderCmd = jgraphCmd + " " + shellQuote(jgrFile.string()) +
			" > " + shellQuote(psFile.string());
		if (system(renderCmd.c_str()) != 0) {
			throw runtime_error("Failed to render PostScript for " +
					jgrFile.string());
		}

		string pdfCmd;
		if (hasPs2pdf) {
			pdfCmd = "ps2pdf " + shellQuote(psFile.string()) + " " +
				shellQuote(pdfFile.string());
		} else {
			pdfCmd = "pstopdf " + shellQuote(psFile.string()) + " -o " +
				shellQuote(pdfFile.string());
		}

		if (system(pdfCmd.c_str()) != 0) {
			throw runtime_error("Failed to render PDF for " + psFile.string());
		}

		string jpgCmd;
		if (hasMagick) {
			jpgCmd = "magick -density 180 " +
				shellQuote(pdfFile.string() + "[0]") + " -quality 92 " +
				shellQuote(jpgFile.string());
		} else {
			jpgCmd = "convert -density 180 " +
				shellQuote(pdfFile.string() + "[0]") + " -quality 92 " +
				shellQuote(jpgFile.string());
		}

		if (system(jpgCmd.c_str()) != 0) {
			throw runtime_error("Failed to render JPG for " + pdfFile.string());
		}
	}
}

// Prints CLI usage and dependency notes.
void printUsage(const string& programName) {
	cerr << "Usage: " << programName << " <bubble|merge|quick> <values...>\n";
	cerr << "Examples:\n";
	cerr << "  " << programName << " bubble 5 1 4 2 8\n";
	cerr << "  " << programName << " merge 38,27,43,3,9,82,10\n";
	cerr << "  " << programName << " quick 10 7 8 9 1 5\n";
	cerr << "Note: Requires jgraph, ps2pdf (or pstopdf), and convert/magick "
		"for JPG conversion.\n";
}


int main(int argc, char* argv[]) {
	if (argc < 4) {
		printUsage(argv[0]);
		return 1;
	}

	try { //error catching for invalid input 
		string sortType = argv[1];
		vector<int> values = parseValues(argc, argv);

		vector<SortStep> steps;
		if (sortType == "bubble") {
			steps = bubbleSortSteps(values);
		} else if (sortType == "merge") {
			steps = mergeSortSteps(values);
		} else if (sortType == "quick") {
			steps = quickSortSteps(values);
		} else {
			throw runtime_error(
					"Sort type must be 'bubble', 'merge', or 'quick'.");
		}

		filesystem::path root = "tests/bin";
		filesystem::path runDir = createRunDir(sortType, root);
		int count = writeSteps(runDir, sortType, steps);
		convertJgraphs(runDir);

		cout << "Generated " << count << " .jgr files\n";
		cout << "Generated " << count << " .ps files\n";
		cout << "Generated " << count << " .pdf files\n";
		cout << "Generated " << count << " .jpg files\n";
		cout << "Output directory: " << runDir.string() << "\n";
		return 0;
	} catch (const exception& error) {
		cerr << "Error: " << error.what() << "\n";
		return 1;
	}
}
