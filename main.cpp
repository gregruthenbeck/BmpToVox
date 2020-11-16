



#include <iostream>
#include <iomanip>
#include <boost/program_options.hpp>
#include <boost/program_options/parsers.hpp>
#include <boost/filesystem.hpp>
#include "easybmp/easybmp.h"
#include "VertPool.h"

using namespace std;
namespace po = boost::program_options;
namespace fs = boost::filesystem;

const bool IsBlack(const RGBApixel* const pix)
{
	return (pix->Red == 0 && pix->Green == 0 && pix->Blue == 0);
}

const float PixColourAsFlt(const RGBApixel* const pix)
{
	return (static_cast<float>(pix->Red)  /255.0f +
			static_cast<float>(pix->Green)/255.0f +
			static_cast<float>(pix->Blue) /255.0f) / 3.0f;
}

const short PixColourAsShort(const RGBApixel* const pix)
{
	return (pix->Red + pix->Green + pix->Blue) / 3;
}

const Vec3 IndexToVert(const int x, const int y, const int z)
{
	return Vec3(static_cast<float>(x),
				static_cast<float>(y),
				static_cast<float>(z));
}

struct AABox
{
	Vec3 minima;
	Vec3 maxima;
	bool inside;
};

bool IsInAABox(const Vec3& p, const AABox& box)
{
	const bool i = (p.x > box.minima.x && p.x < box.maxima.x &&
					p.y > box.minima.y && p.y < box.maxima.y &&
					p.z > box.minima.z && p.z < box.maxima.z);
	return (box.inside ? i : !i);
}

int main(int ac, char** av)
{
	// Declare the supported options.
	po::options_description desc("Allowed options (prefix with '--')");
	desc.add_options()
		("help", "produce help message")
		("s", po::bool_switch(), "silent")
		("t", po::value<short>()->default_value(128), "threshold grey-level [0, 255]")
		("n", po::bool_switch(), "invert (negate) the image")
		("i", po::value<string>()->default_value("."), "input folder (sorts contained BMPs)")
		("o", po::value<string>()->default_value("nodes.txt"),   "output file for node data")
		("O", po::value<string>()->default_value("indices.txt"), "output file for indices data")
		("b", po::value<string>()->default_value("boxes.txt"), "optional input file that contains axis-aligned boxes")
	;

	po::variables_map vm;
	po::store(po::parse_command_line(ac, av, desc), vm);
	po::notify(vm);    

	if (vm.count("help")) {
		cout << desc << "\n";
		cout << "BmpToVox. By Greg Ruthenbeck (C) 2014. Flinders University. Built " << __DATE__ << ". " << __TIME__ << ". Version $Rev: 17 $, $Date: 2014-08-28 11:09:25 +0930 (Thu, 28 Aug 2014) $." << endl;
		cout << "This application can be used to generate a cube-lattice (nodes, elements) from bitmaps." << endl;
		cout << "The input folder should contain only bitmaps that are part of the same sequence that are sequentially named." << endl;
		cout << "Example:" << endl;
		cout << "BmpToVox.exe --i MyImageStackBMPFolder --o nodes.txt --O indices.txt" << endl;
		return 1;
	}

	const bool silentArg = vm["s"].as<bool>();
	const bool negateArg = vm["n"].as<bool>();

	//if (vm.count("input-folder"))
	//{
	//	if (!silentArg)
	//			cout << "Input folder: " << vm["i"].as<string>() << endl;
	//} else {
	//	cout << "input-folder not set (must be a folder that contains bitmaps (that are compatible with EasyBmp C++ lib v1.06)).\n";
	//	return 1;
	//}

	//if (vm.count("output-file-nodes"))
	//{
	//	if (!silentArg)
	//		cout << "Output file for node data is: " << vm["output-file-nodes"].as<string>() << endl;
	//} else {
	//	cout << "output-file-nodes not set (e.g. nodes.txt). Use --help. Exiting\n";
	//	return 1;
	//}

	//if (vm.count("output-file-indices"))
	//{
	//	if (!silentArg)
	//		cout << "Output file for indices data is: " << vm["output-file-indices"].as<string>() << endl;
	//} else {
	//	cout << "output-file-indices not set (e.g. indices.txt). Use --help. Exiting\n";
	//	return 1;
	//}


	const string inputFolderName = vm["i"].as<string>();

	try
	{
		if (!fs::exists(fs::path(inputFolderName.c_str())) || !fs::is_directory(fs::path(inputFolderName.c_str())))
		{
			cout << "Input folder not found. Use --help. Exiting..." << endl;
			return 1;
		}
	}
	catch(const fs::filesystem_error& ex)
	{
		cout << ex.what() << endl;
		return 1;
	}

	vector<fs::path> filenames;                                // so we can sort them later
	copy(fs::directory_iterator(fs::path(inputFolderName)), fs::directory_iterator(), back_inserter(filenames));

	vector<fs::path> bitmapFilenames;
	for (auto fname = filenames.begin(); fname != filenames.end(); ++fname)
	{
		if (fname->has_extension() && fname->extension() == ".bmp")
			bitmapFilenames.push_back(*fname);
	}

	if (bitmapFilenames.empty())
	{
		cout << "Error. No bitmaps \".bmp\" files found in input folder. Use --help" << endl;
		return 1;
	}

	const string outputFilenameNodes = vm["o"].as<string>();
	if (fs::exists(fs::path(outputFilenameNodes.c_str())))
	{
		if (!silentArg)
			cout << "Warning. Overwriting existing nodes output file." << endl;
		fs::remove(fs::path(outputFilenameNodes.c_str()));
	}

	const string outputFilenameIndices = vm["O"].as<string>();
	if (fs::exists(fs::path(outputFilenameIndices.c_str())))
	{
		if (!silentArg)
			cout << "Warning. Overwriting existing indices output file." << endl;
		fs::remove(fs::path(outputFilenameIndices.c_str()));
	}

	vector<AABox> groupBoxes;
	const string inputFilenameBoxes = vm["b"].as<string>();
	if (fs::exists(fs::path(inputFilenameBoxes.c_str())))
	{
		ifstream file(inputFilenameBoxes.c_str(), ios::in);
		while (file.good())
		{
			AABox box;
			file >> box.minima.x;
			file >> box.minima.y;
			file >> box.minima.z;
			file >> box.maxima.x;
			file >> box.maxima.y;
			file >> box.maxima.z;
			file >> box.inside;
			if (file.good())
				groupBoxes.push_back(box);
		}
		file.close();
	}
	else
	{
		AABox box = { Vec3(-1E38f, -1E38f, -1E38f), 
					  Vec3(1E38f, 1E38f, 1E38f), true};
		groupBoxes.push_back(box);
	}

	const int numGroups = groupBoxes.size();
	vector<ofstream> fileNodes, fileIndices;
	for (int gi = 0; gi < numGroups; ++gi)
	{
		stringstream nodesNameSS, indicesNameSS;
		nodesNameSS   << outputFilenameNodes << gi << ".txt" << ends;
		indicesNameSS << outputFilenameIndices << gi << ".txt" << ends;
		fileNodes.push_back(ofstream(nodesNameSS.str().c_str(),   ios::out));
		fileIndices.push_back(ofstream(indicesNameSS.str().c_str(), ios::out));

		if (!fileNodes.back().good() && !silentArg)
		{
			cout << "Failed to open nodes output file." << endl;
			return 1;
		}

		if (!fileIndices.back().good() && !silentArg)
		{
			cout << "Failed to open indices output file." << endl;
			return 1;
		}
	}

	// TODO: Vote on the bitmap dimensions and ignore any that aren't that size
	// Hack: Assume all bitmaps in this folder are part of the sequence (and are all the same dimensions)
	int testWidth = 0;
	int testHeight = 0;
	{
		BMP bmp;
		bmp.ReadFromFile( bitmapFilenames[0].generic_string().c_str() );
		testWidth = bmp.TellWidth();
		testHeight = bmp.TellHeight();
	}

	vector<VertPool<SIMPLE_VERTEX> > vertPools;
	for (int i = 0; i < groupBoxes.size(); ++i)
	{
		// HACK: I have changed the keyDim (1st param of ctor) to be the image width to correct a problem with floating-point node indices in the output (28/Aug/2014).
		// The KeyDim should be as large as possible to ensure that the map used by the VertPool has multiple entries per keyed-block. Otherwise there
		// is no benefit for using the map<>. TODO: Investigate and fix so that map<> behaves correctly and performance is maximized for large datasets.
		vertPools.push_back(VertPool<SIMPLE_VERTEX>((int)((float)testWidth*1.2f), Vec3(-1.0f, -1.0f, -1.0f), 
													Vec3(static_cast<float>(testWidth)  + 2.0f, 
														 static_cast<float>(testHeight) + 2.0f, 
														 static_cast<float>(bitmapFilenames.size()) + 2.0f)));
	}

	sort(bitmapFilenames.begin(), bitmapFilenames.end());

	const short threshold = vm["t"].as<short>();

	unsigned int voxelCount = 0;
	unsigned int sliceCount = 0;
	const string sep = ",\t";
	for (auto str = bitmapFilenames.begin(); str != bitmapFilenames.end(); ++str, ++sliceCount)
	{
		if (!silentArg && sliceCount % 100 == 99)
			cout << "Processing slice " << (sliceCount+1) << " of " << bitmapFilenames.size() << endl;

		BMP bmp;
		if (!bmp.ReadFromFile(str->generic_string().c_str()))
		{
			cout << "Error reading bitmap. Filename = \"" << *str << "\"" << endl;
			continue;
		}
		
		for (int gi = 0; gi < groupBoxes.size(); ++gi)
		{
			VertPool<SIMPLE_VERTEX>& vertPool = vertPools[gi];
			const AABox& box = groupBoxes[gi];
			ofstream& fileInds = fileIndices[gi];
			for (int y = 0; y < bmp.TellHeight(); ++y)
			{
				for (int x = 0; x < bmp.TellWidth(); ++x)
				{
					const short pix = PixColourAsShort(bmp(x,  y));
					if ((pix > threshold) || (negateArg && (pix < threshold)))
					{
						const SIMPLE_VERTEX verts[8] = {SIMPLE_VERTEX(IndexToVert(x,   y,   sliceCount)),
														SIMPLE_VERTEX(IndexToVert(x+1, y,   sliceCount)),
														SIMPLE_VERTEX(IndexToVert(x,   y+1, sliceCount)),
														SIMPLE_VERTEX(IndexToVert(x+1, y+1, sliceCount)),
														SIMPLE_VERTEX(IndexToVert(x,   y,   sliceCount+1)),
														SIMPLE_VERTEX(IndexToVert(x+1, y,   sliceCount+1)),
														SIMPLE_VERTEX(IndexToVert(x,   y+1, sliceCount+1)),
														SIMPLE_VERTEX(IndexToVert(x+1, y+1, sliceCount+1))};
						
						if (!IsInAABox(verts[0].pos, groupBoxes[gi]))
							continue;

						VertIdType indices[8] = { vertPool.AddVertRef(verts[0]),	// 1
												  vertPool.AddVertRef(verts[1]),	// 2
												  vertPool.AddVertRef(verts[2]),	// 3
												  vertPool.AddVertRef(verts[3]),	// 4
												  vertPool.AddVertRef(verts[4]),	// 5
												  vertPool.AddVertRef(verts[5]),	// 6
												  vertPool.AddVertRef(verts[6]),	// 7
												  vertPool.AddVertRef(verts[7])};	// 8
						fileInds << "\t" << (voxelCount+1) << sep << (1+indices[0]) << sep <<
																	 (1+indices[1]) << sep <<
																	 (1+indices[3]) << sep <<
																	 (1+indices[2]) << sep <<
																	 (1+indices[4]) << sep <<
																	 (1+indices[5]) << sep <<
																	 (1+indices[7]) << sep <<
																	 (1+indices[6]) << endl;
						++voxelCount;
					}
				}
			}
		}
	}

	for (int gi = 0; gi < groupBoxes.size(); ++gi)
	{
		fileIndices[gi].flush();
		fileIndices[gi].close();
	}

	for (int i = 0; i < vertPools.size(); ++i)
	{
		unsigned int nodeCount = 0;
		const VertPool<SIMPLE_VERTEX>& vertPool = vertPools[i];
		auto pooledVerts = vertPool.GetPooledVerts();
		ofstream& fileNs = fileNodes[i];
		for (auto v = pooledVerts.begin(); v != pooledVerts.end(); ++v, ++nodeCount)
		{
			fileNs << "\t" << (nodeCount+1) << sep << v->pos.x << sep << v->pos.y << sep << v->pos.z << endl;
		}
		fileNs.flush();
		fileNs.close();
	}

	if (!silentArg)
		cout << "Done. Processing of " << bitmapFilenames.size() << " bitmap(s) completed." << endl;
}

// EOF