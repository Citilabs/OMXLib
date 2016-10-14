#include <OmxCommon.hpp>
#include <OmxAttributeCollection.hpp>
#include <OmxFile.hpp>
#include <OmxMatrix.hpp>
#include <OmxZonalReference.hpp>

#include <memory>
#include <iostream>
#include <string>
#include <stdexcept>
#include <functional>
#include <chrono>

#include <cstdlib> 
#include <ctime> 
#include <cmath>

using namespace std::chrono;

typedef std::function<omx::OmxDouble(const omx::OmxIndex)> seq_value_func;

inline std::string getTestZonalReferenceZoneString(omx::OmxIndex zone) {
	return std::string("Zone Label for #") + std::to_string(zone);
}

inline std::string getTestZonalReferenceName() {
	return std::string("Basic Zonal Reference");
}

bool readWriteTestMatrixAttributes(bool write, omx::OmxMatrix &matrix, omx::OmxIndex matrixNumber) {
	omx::OmxString title("Title");
	omx::OmxString titleValue("Title for Matrix #" + std::to_string(matrixNumber));

	omx::OmxString intAttribute("An Int32 Attribute");
	omx::OmxInt32 intValue(90 + (omx::OmxInt32)matrixNumber);

	omx::OmxString doubleAttribute("A Double Attribute");
	omx::OmxDouble doubleValue(1.89 + matrixNumber);

	bool r = false;

	try {
		if (write) {
			auto& a = matrix.attributes();
			a.setAttribute(title, &titleValue);
			a.setAttribute(intAttribute, &intValue);
			a.setAttribute(doubleAttribute, &doubleValue);

			r = true;
		}
		else {
			omx::OmxString outputString;
			omx::OmxInt32 outputInt;
			omx::OmxDouble outputDouble;

			int mismatches = 0;
			matrix.attributes().getAttribute(title, &outputString);
			if (titleValue != outputString)
				mismatches++;

			matrix.attributes().getAttribute(intAttribute, &outputInt);
			if (intValue != outputInt)
				mismatches++;

			matrix.attributes().getAttribute(doubleAttribute, &outputDouble);

			if (std::abs(doubleValue - outputDouble) > 0.001)
				mismatches++;

			if (mismatches == 0)
				r = true;

		}
	}
	catch (std::exception &e) {
		std::cout << "!! Exception !! : " << e.what() << std::endl;
	}

	return r;
}

void writeMatrix(std::string filename, omx::OmxIndex numZones, omx::OmxIndex numMatrices, omx::OmxCompressionLevel compressionLevel, bool withZonalReference, seq_value_func f) {
	std::remove(filename.c_str());
	omx::OmxFile omx(filename);
	omx.openWithTruncate(numZones);
	
	for (omx::OmxIndex k = 0; k < numMatrices; k++) {
		auto& m = omx.addMatrix("matrix" + std::to_string(k + 1), omx::OmxDataType::Double, compressionLevel);
		auto zones = m.getZones();
		omx::OmxIndex sequenceNum = 0;
		double v = 0;

		if (!readWriteTestMatrixAttributes(true, m, k)) {
			std::cout << "** Unable to write matrix attributes for this trial on matrix #" << k << "." << std::endl;
		}

		std::unique_ptr<double[]> rowBuffer((double *)m.createMatrixRowBuffer());
		auto w = rowBuffer.get();
		for (uint64_t row = 0; row < zones; row++) {
			for (uint64_t col = 0; col < zones; col++, v++, sequenceNum++) {
				rowBuffer[col] = f(sequenceNum);
			}

			m.writeRow(row, w);
		}

	}

	if (withZonalReference) {
		auto zonalReferenceName = getTestZonalReferenceName();
		auto& zonalReference = omx.addZonalReference(zonalReferenceName, omx::OmxDataType::String);

		auto zones = omx.getZones();
		std::vector<std::string> referenceValues;

		for (omx::OmxIndex i = 0; i < zones; i++) {
			referenceValues.push_back(getTestZonalReferenceZoneString(i));
		}

		zonalReference.writeStringReference(referenceValues);
	}

	omx.close();
}

void readMatrix(std::string filename, bool withZonalReference, seq_value_func f) {
	omx::OmxFile omx(filename);
	omx.openReadOnly();

	auto matrixCount = omx.getMatrixCount();
	auto zones = omx.getZones();
	std::cout << "|  Found " << matrixCount << " " << (matrixCount > 1 ? "matrices" : "matrix") << " with " << zones << " zones." << std::endl;

	auto attributes = omx.attributes().getAttributeNames();
	std::cout << "|  Found " << attributes.size() << " attributes." << std::endl;
	for (auto &a : attributes) {
		std::string quote = "";
		if (omx.attributes().getAttributeDataType(a) == omx::OmxDataType::String)
			quote += "\"";

		std::cout << "|    " <<  a << " = " << quote << omx.attributes().getAttributeString(a) << quote << std::endl;
	}

	omx::OmxIndex n = 0;
	for (omx::OmxIndex k = 0; k < matrixCount; k++) {
		auto& m = omx.getMatrix(k);

		if (!readWriteTestMatrixAttributes(false, m, k)) {
			std::cout << "** Verification of read matrix attributes failed for this trial on matrix #" << k << "." << std::endl;
		}

		std::unique_ptr<omx::OmxDouble> rowBuffer((double *)m.createMatrixRowBuffer());
		auto rowData = rowBuffer.get();

		for (omx::OmxIndex row = 0; row < zones; row++) {
			m.readRow(row, rowBuffer.get());
			for (omx::OmxIndex col = 0; col < zones; col++, n++) {
				if (rowData[col] != f(n)) {
					std::cout << "** Verification of read failed for this trial on matrix #" << k << " and row #" << row << "." << std::endl;
					return;
				}
			}
		}
	}

	if (withZonalReference) {
		auto zonalReferenceName = getTestZonalReferenceName();
		auto& zonalReference = omx.getZonalReference(zonalReferenceName);

		auto zones = omx.getZones();
		auto referenceValues = zonalReference.readStringReference();

		for (omx::OmxIndex i = 0; i < zones; i++) {
			if (referenceValues[i] != getTestZonalReferenceZoneString(i)) {
				std::cout << "** Verification of read failed for zonal references on zone #" << i << "." << std::endl;
				return;
			}
		}
	}
	omx.close();
}

struct trial_t {
	std::string trialName;
	bool isDefault;
	bool testZonalReference;
	std::string matrixName;
	omx::OmxCompressionLevel compressionLevel;
	omx::OmxIndex zones;
	omx::OmxIndex matrixCount;
	microseconds duration;
	seq_value_func f;
};

std::vector<trial_t> menu(std::vector<trial_t> &trials) {
	uint32_t selection = -1;
	std::vector<trial_t> selectedTrials;

	std::cout << "======================================================" << std::endl;
	std::cout << "|  Menu" << std::endl;
	std::cout << "------------------------------------------------------" << std::endl;
	std::cout << "|  Enter the number corresponding to the menu option" << std::endl;

	std::cout << "|  1) Run default trials" << std::endl;
	std::cout << "|  2) Run default trials with zonal references" << std::endl;
	std::cout << "|  3) Run trials by selecting characteristics" << std::endl;
	std::cout << "|  4) Run all trials (may take considerable time)" << std::endl;
	std::cout << "|  5) Run trials by selecting from a list" << std::endl;
	std::cout << "------------------------------------------------------" << std::endl;
	std::cout << "|  Enter Selection | ";
	std::cin >> selection;
	std::cout << "------------------------------------------------------" << std::endl;

	auto yesNoFn = []() {
		std::string answer;
		bool haveAnswer = false;
		do {
			std::cin >> answer;
			if (answer != "y" && answer != "n")
				std::cout << "| Invalid response. Enter 'y' or 'n': " << std::endl;
			else
				haveAnswer = true;
		} while (!haveAnswer);

		return answer == "y";
	}; 


	if (selection == 1 || selection == 2) {
		bool withZonalReferences = false;
		if (selection == 1) 
			std::cout << "|  Selected default trials only." << std::endl;
		else {
			std::cout << "|  Selected default trials only with zonal references." << std::endl;
			
			withZonalReferences = true;
		}

		for (auto &trial : trials) {
			if (withZonalReferences)
				trial.testZonalReference = withZonalReferences;

			if (trial.isDefault)
				selectedTrials.push_back(trial);
		}
	}
	else if (selection == 3) {
		omx::OmxIndex maxZones = 500;
		bool withCompression, withoutCompression;
		std::cout << "|  Selecting trials by characteristics ..." << std::endl;
		std::cout << "------------------------------------------------------" << std::endl;
		
		std::vector<omx::OmxIndex> zoneSizes{ 1000, 5000, 10000, 32000 }; 
		for (auto &z : zoneSizes) {
			std::cout << "|  Run trials with up to " << z << " zones? (y/n): ";
			if (yesNoFn()) {
				maxZones = z;
			}
			else {
				break;
			}
		}

		std::cout << "|  Run trials without compression? (y/n): ";
		withoutCompression = yesNoFn();

		std::cout << "|  Run trials with compression? (y/n): ";
		withCompression = yesNoFn();

		for (auto &trial : trials) {
			if (trial.zones <= maxZones
				&& ((withoutCompression && trial.compressionLevel == omx::OmxCompressionLevel::NoCompression)
					|| (withCompression && trial.compressionLevel != omx::OmxCompressionLevel::NoCompression))) {
				selectedTrials.push_back(trial);
			}
		}
	}
	else if (selection == 3) {
		selectedTrials = trials;
	}
	else if (selection == 4) {
		std::cout << "**** Not implemented" << std::endl;
		std::exit(-1);
	}
	else {
		std::cout << "**** Invalid selection" << std::endl;
		std::exit(-1);
	}

	return selectedTrials;
}

int main(int argc, char *argv[]) {
	if (argc < 2) {
		std::cout << "No output directory specified." << std::endl;
		return -1;
	}

	std::string rootPath(argv[1]);

	std::vector<trial_t> trials{
		// general
		{ "Sequential values with no compression", true, false, "5K.omx", omx::OmxCompressionLevel::NoCompression, 5000, 1, microseconds(), [](const omx::OmxIndex n) { return (omx::OmxDouble)n; } },
		{ "Sequential values with no compression", false, false, "10K.omx", omx::OmxCompressionLevel::NoCompression, 10000, 1, microseconds(), [](const omx::OmxIndex n) { return (omx::OmxDouble)n; } },
		{ "Sequential values with no compression", false, false, "32K.omx", omx::OmxCompressionLevel::NoCompression, 32000, 1, microseconds(), [](const omx::OmxIndex n) { return (omx::OmxDouble)n; } },
		{ "Sequential doubled values with no compression", true, false, "5K_doubled.omx", omx::OmxCompressionLevel::NoCompression, 5000, 1, microseconds(), [](const omx::OmxIndex n) { return (omx::OmxDouble)2 * n; } },
		{ "Random values with no compression", true, false, "5K_random.omx", omx::OmxCompressionLevel::NoCompression, 5000, 1, microseconds(), [](const omx::OmxIndex n) { return (omx::OmxDouble)std::rand(); } },
		{ "Sequential values with no compression", false, false, "10K.omx", omx::OmxCompressionLevel::NoCompression, 10000, 1, microseconds(), [](const omx::OmxIndex n) { return (omx::OmxDouble)n; } },
		{ "Sequential values with no compression", false, false, "32K.omx", omx::OmxCompressionLevel::NoCompression, 32000, 1, microseconds(), [](const omx::OmxIndex n) { return (omx::OmxDouble)n; } },

		// 500 zones
		{ "Sequential values with no compression", false, false, "0.5K.omx", omx::OmxCompressionLevel::NoCompression, 500, 1, microseconds(), [](const omx::OmxIndex n) { return (omx::OmxDouble)n; } },
		{ "Sequential values with compression", false, false, "0.5K_compressed.omx", omx::OmxCompressionLevel::Level_4, 500, 1, microseconds(), [](const omx::OmxIndex n) { return (omx::OmxDouble)n; } },
		{ "Sequential doubled values with no compression", false, false, "0.5K_doubled.omx", omx::OmxCompressionLevel::NoCompression, 500, 1, microseconds(), [](const omx::OmxIndex n) { return (omx::OmxDouble)2 * n; } },
		{ "Sequential doubled values with compression", false, false, "0.5K_doubled_compressed.omx", omx::OmxCompressionLevel::Level_4, 500, 1, microseconds(), [](const omx::OmxIndex n) { return (omx::OmxDouble)2 * n; } },
		{ "Random values with no compression", false, false, "0.5K_random.omx", omx::OmxCompressionLevel::NoCompression, 500, 1, microseconds(), [](const omx::OmxIndex n) { return (omx::OmxDouble)std::rand(); } },
		{ "Random values with compression", false, false, "0.5K_random_compressed.omx", omx::OmxCompressionLevel::Level_4, 500, 1, microseconds(), [](const omx::OmxIndex n) { return (omx::OmxDouble)std::rand(); } },

		// 1000 zones
		{ "Sequential values with no compression", false, false, "1K.omx", omx::OmxCompressionLevel::NoCompression, 1000, 1, microseconds(), [](const omx::OmxIndex n) { return (omx::OmxDouble)n; } },
		{ "Sequential values with compression", false, false, "1K_compressed.omx", omx::OmxCompressionLevel::Level_4, 1000, 1, microseconds(), [](const omx::OmxIndex n) { return (omx::OmxDouble)n; } },
		{ "Sequential doubled values with no compression", false, false, "1K_doubled.omx", omx::OmxCompressionLevel::NoCompression, 1000, 1, microseconds(), [](const omx::OmxIndex n) { return (omx::OmxDouble)2 * n; } },
		{ "Sequential doubled values with compression", false, false, "1K_doubled_compressed.omx", omx::OmxCompressionLevel::Level_4, 1000, 1, microseconds(), [](const omx::OmxIndex n) { return (omx::OmxDouble)2 * n; } },
		{ "Random values with no compression", false, false, "1K_random.omx", omx::OmxCompressionLevel::NoCompression, 1000, 1, microseconds(), [](const omx::OmxIndex n) { return (omx::OmxDouble)std::rand(); } },
		{ "Random values with compression", false, false, "1K_random_compressed.omx", omx::OmxCompressionLevel::Level_4, 1000, 1, microseconds(), [](const omx::OmxIndex n) { return (omx::OmxDouble)std::rand(); } },

		// general with compression
		{ "Sequential values with compression", false, false, "5K_compressed.omx", omx::OmxCompressionLevel::Level_4, 5000, 1, microseconds(), [](const omx::OmxIndex n) { return (omx::OmxDouble)n; } },
		{ "Sequential values with compression", false, false, "10K_compressed.omx", omx::OmxCompressionLevel::Level_4, 10000, 1, microseconds(), [](const omx::OmxIndex n) { return (omx::OmxDouble)n; } },
		{ "Sequential values with compression", false, false, "32K_compressed.omx", omx::OmxCompressionLevel::Level_4, 32000, 1, microseconds(), [](const omx::OmxIndex n) { return (omx::OmxDouble)n; } },
		{ "Sequential doubled values with compression", false, false, "5K_doubled_compressed.omx", omx::OmxCompressionLevel::Level_4, 5000, 1, microseconds(), [](const omx::OmxIndex n) { return (omx::OmxDouble)2 * n; } },
		{ "Random values with compression", false, false, "5K_random_compressed.omx", omx::OmxCompressionLevel::Level_4, 5000, 1, microseconds(), [](const omx::OmxIndex n) { return (omx::OmxDouble)std::rand(); } },
		{ "Sequential values with compression", false, false, "10K_compressed.omx", omx::OmxCompressionLevel::Level_4, 10000, 1, microseconds(), [](const omx::OmxIndex n) { return (omx::OmxDouble)n; } },
		{ "Sequential values with compression", false, false, "32K_compressed.omx", omx::OmxCompressionLevel::Level_4, 32000, 1, microseconds(), [](const omx::OmxIndex n) { return (omx::OmxDouble)n; } },
	};
	trials = menu(trials);
	uint32_t i = 1;
	auto performTrial = [](std::function<void()> f) {

		high_resolution_clock::time_point t1 = high_resolution_clock::now();
		try {
			f();
		}
		catch (std::runtime_error& ex) {
			std::cout << "runtime error: " << ex.what() << std::endl;
		}
		catch (std::exception& ex){
			std::cout << "exception: " << ex.what() << std::endl;
		}
		catch (...) {
			std::cout << "default exception" << std::endl;
		}
		high_resolution_clock::time_point t2 = high_resolution_clock::now();
		auto duration = std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1).count();
		std::cout << "|  =====================> time: " << (double)duration / 1000000.0 << " seconds" << std::endl;
	};

	for (auto& trial : trials) {
		auto seed = (uint32_t)std::time(NULL);
		auto filename = rootPath + trial.matrixName;
		auto writeFn = [&filename, &trial](){ writeMatrix(filename, trial.zones, trial.matrixCount, trial.compressionLevel, trial.testZonalReference, trial.f); };
		auto readFn = [&filename, &trial, &trials](){ readMatrix(filename, trial.testZonalReference, trial.f); };
		
		std::cout << "======================================================" << std::endl;
		std::cout << "|Trial " << i << ": " << trial.trialName << std::endl;
		std::cout << "------------------------------------------------------" << std::endl;
		std::cout << "|  filename: " << filename << std::endl;
		std::cout << "------------------------------------------------------" << std::endl;
		std::cout << "|  " << trial.matrixCount << " " << (trial.matrixCount > 1 ? "matrices" : "matrix") << " with " << trial.zones << " zone" << (trial.zones > 1 ? "s" : "") << std::endl;
		std::cout << "------------------------------------------------------" << std::endl;

		std::cout << "|  Running WRITE phase ..." << std::endl;
		std::srand(seed);
		performTrial(writeFn);
		std::cout << "|  WRITE phase complete." << std::endl;

		std::cout << "|" << std::endl;

		std::cout << "|  Running READ phase ..." << std::endl;
		std::srand(seed);
		performTrial(readFn);
		std::cout << "|  READ phase complete." << std::endl;

		std::cout << "======================================================" << std::endl << std::endl << std::endl;

		i++;
	}

	std::cout << "Trials complete." << std::endl;
	return 0;
}