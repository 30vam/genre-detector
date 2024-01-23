#include <filesystem>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <regex>
#include <functional>
#include <algorithm>
#include <map>
#include <vector>

using namespace std;

//Stroy Class for saving all information about a story object in memory:
struct Story
{
	string storyName;
	//Each of these vectors store story words related to each genre and their repeat count
	vector<string> storyWords;
	vector<pair<string, pair<int, int>>> romanceKeywords;  //vector.first contains keywordname, vector.second.first is repeat count and vector.second.second is keyword weight
	vector<pair<string, pair<int, int>>> mysteryKeywords;
	vector<pair<string, pair<int, int>>> fantasyKeywords;
	vector<pair<string, pair<int, int>>> sciFiKeywords;
	//vector.first stores genre name, vector.second.first is confidence PERCENT
	vector<pair<string, double>> confidenceVector = { {"romance", 0}, {"mystery", 0}, {"fantasy", 0}, {"scifi", 0} };
	vector<string> topFiveKeywords; 
	string estimatedGenre;
};

//Vectors that contain story lists
vector<Story> importedStories;
vector<Story> analyzedStories;  

//Functions and maps etc.
void loadFile(string fileName, map <string, int>& dataMap);
void loadFile(string fileName, Story& storyObject);
void getUserInput();
bool areStringsEqualIgnoreCase(string str1, string str2);

map <string, int> romanceMap;  //The data from thecsv  files are stored in a map
map <string, int> mysteryMap;
map <string, int> fantasyMap;
map <string, int> sciFiMap;

//A static utility class that stores functions, lists etc. related to all the user commands:
class Commands 
{
public: 
	static vector<pair<string, function<void(vector<string> otherInputs)>>> commandVector;

	static void showTheListOfCommands(vector<string> otherInputs)
	{
		if (otherInputs.size() != 0)  //Check if the user has entered other text after the command name
			cout << "Command not found. See the list of commands with show_the_list_of_commands." << endl << endl;
		else
		{
			for (auto const& pair : commandVector)
				cout << pair.first << endl;
		}
	}

	static void importStory(vector<string> otherInputs)
	{
		if (otherInputs.size() != 1)  //Check if the user has entered command arguments after the command name correctly
			cout << "import_story {filename.txt}" << endl << endl;
		else  //Do these if the command was entered correctly
		{
			try //Try to Save the story text in a variable 
			{
				//Create a new story object
				Story newStoryObj = Story(); 

				loadFile(otherInputs[0], newStoryObj);

				//Story name shouldn't contain extensions
				char extensionStartingIndex = otherInputs[0].find_last_of(".");
				string storyName = otherInputs[0].substr(0, extensionStartingIndex);  //remove .txt from file name if it exists
				newStoryObj.storyName = storyName;

				if (newStoryObj.storyWords.size() != 0)  //Show a message for successful story import, which is when the story object has a non empty name
					cout << newStoryObj.storyName << " imported successfully." << endl;

				importedStories.push_back(newStoryObj);  //Add the story to imported stories list

				//Test
				//cout << newStoryObj.getStoryText() << endl;
			}
			catch (const char* msg) //Catch an error if the file cant be opened/doesnt exist
			{
				cerr << msg << endl;
			}
		}
	}

	static void showTheListOfStories(vector<string> otherInputs)
	{
		if (otherInputs.size() != 0) 
			cout << "Command not found. See the list of commands with show_the_list_of_commands." << endl << endl;
		else
		{
			int storyIndex = 1;
			string storyName;
			cout << "List of all imported stories:" << endl;

			for (int i = 0; i < importedStories.size(); i++)
			{
				//Get the story name and capitalize the 1st letter
				storyName = importedStories[i].storyName;
				storyName[0] = toupper(storyName[0]);  //First letter shoud be capital

				for (int i = 1; i < storyName.length(); i++)  //The remaining letters should be small
					storyName[i] = tolower(storyName[i]);

				cout << to_string(storyIndex) << ". " << storyName << endl;
				storyIndex++;
			}
		}
	}

	static void analyzeStory(vector<string> otherInputs)
	{
		if (otherInputs.size() != 2) 
			cout << "analyze_story {story_index} {output_file_name.txt}" << endl << endl;
		else
		{
			int storyIndex;

			if (!Commands::is_number(otherInputs[0]))  //Show this message if it was text instead of number
				cout << "Invalid story index." << endl;
			else 
			{
				storyIndex = stoi(otherInputs[0]);  //if the string of index entered by the user is actually a number, convert it to an int 

				//Check if the story index entered by user is in valid range
				if (storyIndex < 1 || storyIndex > importedStories.size())
					cout << "Invalid story index." << endl;
				else  //Start the operation after the story index was checked
				{
					Story& selectedStory = importedStories[storyIndex - 1];  //Using a reference(&) of the story object instead of a COPY
					//Count words and confidence etc. of each genre
					Commands::countGenreWords(selectedStory, romanceMap, selectedStory.romanceKeywords);
					Commands::countGenreWords(selectedStory, mysteryMap, selectedStory.mysteryKeywords);
					Commands::countGenreWords(selectedStory, fantasyMap, selectedStory.fantasyKeywords);
					Commands::countGenreWords(selectedStory, sciFiMap, selectedStory.sciFiKeywords);
					Commands::calclateConfidence(selectedStory);
					Commands::calculateGenre(selectedStory);
					Commands::calculateCommonKeywords(selectedStory);

					analyzedStories.push_back(selectedStory);

					cout << "The genre of the story " << selectedStory.storyName << " is " << selectedStory.estimatedGenre << "." << endl << endl;

					try  //Try to create an output file
					{
						Commands::createTextOutput(selectedStory, otherInputs[1]);
					}
					catch (const char* msg)
					{
						cerr << msg << endl;
					}

					//Test
					/*for (int i = 0; i < selectedStory.romanceKeywords.size(); i++)
					{
						if (i == 0)
							cout << "*****Romance keyword : Repeat count : Keyword weight*****" << endl;
						cout << selectedStory.romanceKeywords[i].first << " : " << selectedStory.romanceKeywords[i].second.first << " : " << selectedStory.romanceKeywords[i].second.second << endl;
					}
					for (int i = 0; i < selectedStory.mysteryKeywords.size(); i++)
					{
						if (i == 0)
							cout << "*****Mystery keyword : Repeat count : Keyword weight*****" << endl;
						cout << selectedStory.mysteryKeywords[i].first << " : " << selectedStory.mysteryKeywords[i].second.first << " : " << selectedStory.mysteryKeywords[i].second.second << endl;
					}
					for (int i = 0; i < selectedStory.fantasyKeywords.size(); i++)
					{
						if (i == 0)
							cout << "*****Fantasy keyword : Repeat count : Keyword weight*****" << endl;
						cout << selectedStory.fantasyKeywords[i].first << " : " << selectedStory.fantasyKeywords[i].second.first << " : " << selectedStory.fantasyKeywords[i].second.second << endl;
					}
					for (int i = 0; i < selectedStory.sciFiKeywords.size(); i++)
					{
						if (i == 0)
							cout << "*****SciFi keyword : Repeat count : Keyword weight*****" << endl;
						cout << selectedStory.sciFiKeywords[i].first << " : " << selectedStory.sciFiKeywords[i].second.first << " : " << selectedStory.sciFiKeywords[i].second.second << endl;
					}
					//Confidence test
					cout << "---------CONFIDENCE-----------" << endl;
					for (int i = 0; i < selectedStory.confidenceVector.size(); i++)
					{
						cout << selectedStory.confidenceVector[i].first << " confidence percent : " << selectedStory.confidenceVector[i].second << endl;
					}
					//Top keywords
					cout << "Top 5 keywords : ";
					for (auto const& keyword : selectedStory.topFiveKeywords)
					{
						cout << keyword << ", ";
					}
					cout << endl << "Top 4 keywords : ";
					for (auto const& keyword : selectedStory.topFourKeywords)
					{
						cout << keyword << ", ";
					}*/
				}
			}
		}
	}

	static void analyzedStoriesList(vector<string> otherInputs)
	{
		if (otherInputs.size() != 0)
			cout << "Command not found. See the list of commands with show_the_list_of_commands." << endl << endl;
		else
		{
			string storyName;
			cout << "The analyzed stories are: ";
			
			for (int i = 0; i < analyzedStories.size(); i++)
			{
				//Get the story name and capitalize the 1st letter
				storyName = analyzedStories[i].storyName;
				if(i == 0)  //First name should start with capital letter
					storyName[0] = toupper(storyName[0]);
				else  //Other names should be printed with small letters
				{
					for (int i = 0; i < storyName.length(); i++) 
						storyName[i] = tolower(storyName[i]);
				}
				
				if (i == analyzedStories.size() - 1)  //If it's the last name, print . instead of ,
					cout << storyName << "." << endl;
				else if(i == analyzedStories.size() - 2) //Add and before the last word
					cout << storyName << " and ";
				else
					cout << storyName << ", ";
			}
		}
	}

	static void showStoryAnalysis(vector<string> otherInputs)
	{
		if (otherInputs.size() != 1)
			cout << "show_story_analysis {story_index}" << endl << endl;
		else
		{
			int storyIndex;

			if (!Commands::is_number(otherInputs[0]))  //Show this message if it was text instead of number
				cout << "Invalid story index." << endl;
			else
			{
				storyIndex = stoi(otherInputs[0]);  //if the string of index entered by the user is actually a number, convert it to an int

				//Check if the story index entered by user is in valid range
				if (storyIndex < 1 || storyIndex > importedStories.size())
					cout << "Invalid story index." << endl;
				else  //Start the operation after the story index was checked
				{
					Story& selectedStory = importedStories[storyIndex - 1];

					if (selectedStory.estimatedGenre == "")  //Show this message if the story wasn't analyzed. If the story genre wasn't detected, it means the story is not analyzed
						cout << "This story has not been analyzed yet. Please use the analyze_story command." << endl;
					else
						cout << getAnalyticData(selectedStory);
				}
			}
		}
	}

	static void dumpAnalyzedStories(vector<string> otherInputs)
	{
		if (otherInputs.size() != 1)
			cout << "dump_analyzed_stories {output_file_name.csv}" << endl << endl;
		else
		{
			if (analyzedStories.size() == 0)
				cout << "No analyzed stories to dump." << endl;
			else
			{
				try  //try to create a csv file
				{
					createCsvOutput(otherInputs[0]);
					cout << "All analyzed stories dumped in " << otherInputs[0] << "." << endl;
				}
				catch (const char* msg)
				{
					cout << msg << endl;
				}
			}
		}
	}

	static void exitApp(vector<string> otherInputs)
	{
		if (otherInputs.size() != 0)
			cout << "Command not found. See the list of commands with show_the_list_of_commands." << endl << endl;
		else
		{
			exit(0);
		}
	}



	//Other methods:
	//Store genre words and their repeat count in a vector inside the story class
	static void countGenreWords(Story &storyObj, map <string, int> &keywordMap, vector<pair<string, pair<int, int>>> &wordCountVector)
	{
		bool isKeyword = false;
		bool isUnique = true;
		int keywordWeight;
		int wordCount = storyObj.storyWords.size();  //How many words does the story have?

		wordCountVector.clear();  //Clear the keywords saved from before, maybe the user analyzed the same story twice?

		//Compare each word in story to keywords:
		for (int currentWordIndex = 0; currentWordIndex < wordCount; currentWordIndex++)
		{
			string currentWord = storyObj.storyWords[currentWordIndex];
		
			for (auto const& keywordMapItem : keywordMap)  //Compare each genre keyword from csv file to the word from story
			{
				if (keywordMapItem.first == currentWord)  //If the words are the same, then this story word is a keyword
				{
					isKeyword = true;
					keywordWeight = keywordMapItem.second;
					break;
				}
			}

			if (isKeyword)  //if the story word was found in genre keywords:
			{
				int repeatCount = 0;
				for (auto const& storyWord : storyObj.storyWords)  //Now count how many times this word was repeated in the story
				{   
					if (storyWord == currentWord)
						repeatCount++;
				}

				for (auto const& item : wordCountVector)  //Check if the keyword was already saved in the vector
				{
					if (item.first == currentWord)  
					{
						isUnique = false;
						break;
					}
				}

				//save that keyword and its repeat count in a vector inside that story object if the keyword was unique
				if (isUnique)
					wordCountVector.push_back({ currentWord, {repeatCount, keywordWeight } });

				//Sort the vector by repeat count (which is vector.second.first):
				sort(wordCountVector.begin(), wordCountVector.end(), [](const auto& left, const auto& right) 
					{
						return left.second.first > right.second.first;
					});
			}

			isKeyword = false; //Set isKeyword to false and isUnique to true for the next word when the loop ends
			isUnique = true;
			//Test
			//cout << currentWord << endl;
		}
	}
	
	static void calclateConfidence(Story &storyObj)
	{
		//Genre weight = sum of this expression for all keywords : number of keyword repeats * keyword wight
		double romanceWeight = Commands::calculateGenreWeight(storyObj.romanceKeywords);
		double mysteryWeight = Commands::calculateGenreWeight(storyObj.mysteryKeywords);
		double fantasyWeight = Commands::calculateGenreWeight(storyObj.fantasyKeywords);
		double sciFiWeight = Commands::calculateGenreWeight(storyObj.sciFiKeywords);

		double totalWeight = romanceWeight + mysteryWeight + fantasyWeight + sciFiWeight;

		double romanceConfidence = (romanceWeight / totalWeight) * 100;
		double mysteryConfidence = (mysteryWeight / totalWeight) * 100;
		double fantasyConfidence = (fantasyWeight / totalWeight) * 100;
		double sciFiConfidence = (sciFiWeight / totalWeight) * 100;

		storyObj.confidenceVector[0].second = romanceConfidence;  //set romance confidence value inside the story object, romace index is 0
		storyObj.confidenceVector[1].second = mysteryConfidence;
		storyObj.confidenceVector[2].second = fantasyConfidence;
		storyObj.confidenceVector[3].second = sciFiConfidence;
	}

	//Genre weight = sum of this for all keywords : number of keyword repeats * keyword wight
	static double calculateGenreWeight (vector<pair<string, pair<int, int>>>& storyKeywordsVec)
	{
		double genreWeight = 0;

		for (auto const& item : storyKeywordsVec)  //Calculate romance weight
		{
			double repeatCount = item.second.first;
			double wordWeight = item.second.second;

			genreWeight += wordWeight * repeatCount;
		}

		return genreWeight;
	}

	//Guess the genre of a story based on their confidence percent
	static void calculateGenre(Story& storyObj)
	{
		vector<pair<string, double>> confidenceVector = storyObj.confidenceVector;

		//Sort the confidence vector by confidence level (which is pair.second):
		sort(confidenceVector.begin(), confidenceVector.end(), [](const auto& left, const auto& right)
			{
				return left.second > right.second;
			});
		
		storyObj.estimatedGenre = confidenceVector[0].first;  //Also save the genre inside a variable of story obj
	}

	static void calculateCommonKeywords(Story& storyObj)
	{
		string storyGenre = storyObj.estimatedGenre;
		vector<pair<string, pair<int, int>>> keywordsVec;

		//Clear the top keyword vectors first in case user analyzed the same story twice
		storyObj.topFiveKeywords.clear();

		//Depending on the genre of the story, chose which keyword vec needs to be used for extracting top keywords
		if (storyGenre == "romance")
			keywordsVec = storyObj.romanceKeywords;
		else if (storyGenre == "mystery")
			keywordsVec = storyObj.mysteryKeywords;
		else if (storyGenre == "fantasy")
			keywordsVec = storyObj.fantasyKeywords;
		else if (storyGenre == "scifi")
			keywordsVec = storyObj.sciFiKeywords;

		for (size_t i = 0; i < 5; i++)
			storyObj.topFiveKeywords.push_back(keywordsVec[i].first);
	}

	//Count the keyword for each genre
	static int countKeywords(vector<pair<string, pair<int, int>>>& storyKeywordsVec) 
	{
		int keywordCount = 0;

		for (const auto& item : storyKeywordsVec)
		{
			keywordCount += item.second.first;
		}

		return keywordCount;
	}

	static string getAnalyticData(Story& selectedStory)
	{
		string analyticData;
		stringstream keywordsStream;

		for (int i = 0; i < 5; i++)  //store top 5 keywords in a stringstream
		{
			if (i != 4)
				keywordsStream << selectedStory.topFiveKeywords[i] << ", ";
			else  //For the last keyword, print . instead of ,
				keywordsStream << selectedStory.topFiveKeywords[i] << ".";
		}

		//Write information to the file
		analyticData = "Story Name: " + selectedStory.storyName + "\n" +
			"Predicted Genre: " + selectedStory.estimatedGenre + "\n\n\n" +
			"Genre, Number of Keywords, Confidence\n" +
			"Romance, " + to_string(countKeywords(selectedStory.romanceKeywords)) + ", " + to_string(selectedStory.confidenceVector[0].second) + "\n" +
			"Mystery, " + to_string(countKeywords(selectedStory.mysteryKeywords)) + ", " + to_string(selectedStory.confidenceVector[1].second) + "\n" +
			"Fantasy, " + to_string(countKeywords(selectedStory.fantasyKeywords)) + ", " + to_string(selectedStory.confidenceVector[2].second) + "\n" +
			"SciFi, " + to_string(countKeywords(selectedStory.sciFiKeywords)) + ", " + to_string(selectedStory.confidenceVector[3].second) + "\n" +
			"The common keywords of the story are: " + keywordsStream.str();

		return analyticData;
	}

	static void createTextOutput(Story& selectedStory, string userFileName)
	{
		string finalFileName;

		if (userFileName.substr(userFileName.find_last_of(".") + 1) != "txt")  //Check if the file has .txt extension
			finalFileName = userFileName + ".txt";
		else
			finalFileName = userFileName;

		filesystem::path filePath = finalFileName;

		//Check if file already exists
		if (filesystem::exists(filePath))
		{
			//If the file exists, try to remove it and if it can't be removed, throw an error
			ofstream outputFile(finalFileName, ios::out | ios::trunc);
			
			if (!outputFile.is_open())
				throw "A file with the same name already exist, but can't be opened for content removal.";

			outputFile.close();
		}
		//If the file doesn't exist, create it
		ofstream outputFile(finalFileName);

		if (outputFile.is_open())
		{
			//Write information to the file
			outputFile << getAnalyticData(selectedStory);
		}
		else
		{
			throw "Unable to create or open the output file.";
		}

		outputFile.close();  //Close the file at the end
	}

	static void createCsvOutput(string userFileName) 
	{
		string finalFileName;

		if (userFileName.substr(userFileName.find_last_of(".") + 1) != "csv")  //Check if the file has .txt extension
			finalFileName = userFileName + ".csv";
		else
			finalFileName = userFileName;

		filesystem::path filePath = finalFileName;

		//Check if file already exists
		if (filesystem::exists(filePath))
		{
			//If the file exists, try to clear it and if it can't be removed, throw an error
			ofstream outputFile(finalFileName, ios::out | ios::trunc);

			if (!outputFile.is_open())
				throw "A file with the same name already exist, but can't be opened for content removal.";

			outputFile.close();
		}
		//If the file doesn't exist, create it
		ofstream outputFile(finalFileName);

		if (outputFile.is_open())
		{
			//Write information to the file if the file was open
			outputFile << "Story, Genre, Confidence, Romance Words, Mystery Words, Fantasy Words, SciFi Words, Common Keyword 1, Common Keyword 2, Common Keyword 3, Common Keyword 4" << endl;
			

			for (auto story : analyzedStories)  //For each analyzed story, output these data
			{
				double confidence = 0.0;

				if (story.estimatedGenre == "romance")
					confidence = story.confidenceVector[0].second;
				else if (story.estimatedGenre == "mystery")
					confidence = story.confidenceVector[1].second;
				else if (story.estimatedGenre == "fantasy")
					confidence = story.confidenceVector[2].second;
				else if (story.estimatedGenre == "scifi")
					confidence = story.confidenceVector[3].second;

				outputFile << story.storyName << "," << story.estimatedGenre << "," << confidence << "," << to_string(countKeywords(story.romanceKeywords)) << ","
					<< to_string(countKeywords(story.mysteryKeywords)) << "," << to_string(countKeywords(story.fantasyKeywords)) << ","
					<< to_string(countKeywords(story.sciFiKeywords)) << "," << story.topFiveKeywords[0] << "," << story.topFiveKeywords[1] 
					<< "," << story.topFiveKeywords[2] << "," << story.topFiveKeywords[3] << endl;
			}
		}
		else
		{
			throw "Unable to create or open the output file.";
		}

		outputFile.close();  //Close the file at the end
	
	}

	//Checks if a string actually contains numbers
	static bool is_number(const std::string& s)  
	{
		std::string::const_iterator it = s.begin();
		while (it != s.end() && std::isdigit(*it)) ++it;
		return !s.empty() && it == s.end();
	}
};
//This vector connects all commands to their corresponding functions, I used a vector<pair> instead of a map because a map sorts the command names by alphabetical order...
vector<pair<string, function<void(vector<string> otherInputs)>>> Commands::commandVector =
{
	{"show_the_list_of_commands", Commands::showTheListOfCommands},
	{"import_story", Commands::importStory},
	{"show_the_list_of_stories", Commands::showTheListOfStories},
	{"analyze_story", Commands::analyzeStory},
	{"analyzed_stories_list", Commands::analyzedStoriesList},
	{"show_story_analysis", Commands::showStoryAnalysis},
	{"dump_analyzed_stories", Commands::dumpAnalyzedStories},
	{"exit", Commands::exitApp}
};

int main()
{
	try //Try to load the data files
	{
		loadFile("Romance.csv", romanceMap);  //This function simply stores the csv file data in a map for later use
		loadFile("Mystery.csv", mysteryMap);
		loadFile("Fantasy.csv", fantasyMap);
		loadFile("SciFi.csv", sciFiMap);
	}

	catch (const char* msg) //Catch an error if the file cant be opened/doesnt exist and end the application
	{
		cerr << msg << endl;
		return 0;
	}

	//Show the list of commands after the files are loaded successfully
	Commands::showTheListOfCommands(vector<string>());

	while (true)  
	{
		getUserInput();
	}
}

//For CSV files
void loadFile(string fileName, map <string, int> &dataMap)
{
	ifstream file(fileName);  //open the file

	if (file.is_open() && !file.fail() && fileName.substr(fileName.find_last_of(".") + 1) == "csv")   //If the file can be opened and is a csv
	{
		string line;

		if (getline(file, line))  //The first line of the csv file (The headers)
		{
			//Check if the first row (or first line) of the file is in correct formant, if it's not then this is not a CSV file or the formant is not correct
			string firstRow = "Word " + fileName.substr(0, fileName.find_last_of(".")) + ",Coefficient";
			if (!areStringsEqualIgnoreCase(firstRow, line))
			{
				throw "Error importing genre keywords. Please check keyword files.";
			}
		}
		else  //If the first line cant be accessed
		{
			throw "Error importing genre keywords. Please check keyword files.";
		}

		while (getline(file, line))  //Read each line of the file until it ends, and perform actions on each line
		{
			stringstream lineStream(line);
			string keyword;
			string weightString;

			while (lineStream.good())  //As long as the line doesnt have any errors
			{
				if (getline(lineStream, keyword, ','))  //TRY(if) to get the keyword, which is the line string until the "comma" delimiter
				{
					if (getline(lineStream, weightString))  //TRY(if) to get the weight value
					{
						//transform converts all the characters to lowercase
						transform(keyword.begin(), keyword.end(), keyword.begin(), ::tolower);
						dataMap.insert(pair<string, int>(keyword, stoi(weightString)));
					}
				}
			}
		}

		file.close();  //Close the file when there are no more lines when we are finished
	}
	else if (!file.is_open())  //Throw an error if a csv file can not be opened of doesn't exist or can't be read
	{
		throw "Error importing genre keywords. Please check keyword files.";
	} 

	//Test if the maps are correct
	/*int wordCount = 0;
	for (auto& pair : dataMap)
	{
		cout << pair.first << " : " << pair.second << endl;
		wordCount++;
	}
	cout << "Word Count: " << wordCount << endl;*/
}

//Overload of the function for txt files
void loadFile(string fileName, Story& storyObject)
{
	//First check if the file exists at all
	filesystem::path filePath = fileName;

	if (!filesystem::exists(filePath))
		throw "File not found.";
	else
	{
		ifstream file(fileName);  //open the file

		if (file.is_open())  //Check if the file is open
		{
			if (file.fail() || fileName.substr(fileName.find_last_of(".") + 1) != "txt")  //if the file is found, but is not a text file or has a problem throw an error
				throw "Error importing the story.";
			else
			{
				string line;
				string fileText;

				while (getline(file, line))  //save the story in fileText variable
				{
					fileText += line;
				}

				//remove ,  . " ' from the story text because they are not part of words
				regex pattern("[,\"\'.]");
				fileText = regex_replace(fileText, pattern, " ");

				//Now save each UNIQUE word of the story inside a vector
				vector<string> words;
				istringstream storyStream(fileText);  //Create a istringstream from story text

				while (storyStream)  //As long as story text has a word, store each word in a vector
				{
					string currentWord;
					storyStream >> currentWord;

					transform(currentWord.begin(), currentWord.end(), currentWord.begin(), ::tolower);
					words.push_back(currentWord);
				}

				storyObject.storyWords = words;

				//Test
				/*vector<string> storyWords = storyObject.storyWords;
				for (auto& pair : storyWords)
				{
					cout << pair << endl;
				}*/
			}

			file.close();  //Close the file when there are no more lines when we are finished
		}
		else  //check if the file exists but cant be read or opened
		{
			throw "Error importing the story.";
		}
	}
}

void getUserInput() 
{
	string userInputLine;  //This variable stores the entire line the user enters, not just the first word which is the command name
	bool doesCommandExist = false;

	if (getline(cin, userInputLine)) 
	{
		istringstream userInputStream(userInputLine);
		vector<string> userInputWords;

		//Seperate input line by words, and store all input words inside a vector
		string word;
		while (userInputStream >> word)
		{
			if (word != "")
				userInputWords.push_back(word);
		}

		for (auto const& pair : Commands::commandVector)  //Try to find which function is related to the command entered by user and invoke it(The first word of the input vector is the command name)
		{
			if (userInputWords.size() == 0)  //break out of the function and show help if no command was entered and only enter was pressed
				return;

			if (pair.first == userInputWords[0])
			{
				userInputWords.erase(userInputWords.begin());  //Remove the command name from the vector, so the vector only contains needed command arguments now.
				
				pair.second(userInputWords);  //Call to the function related to the entered command
				doesCommandExist = true;
				break;
			}
		}

		//If the command wasn't found, show help:
		if ((!doesCommandExist))
			cout << "Command not found. See the list of commands with show_the_list_of_commands." << endl;
	}
}

//Checks wether 2 strings are the same WITHOUT case sensitivity
bool areStringsEqualIgnoreCase(string str1, string str2) 
{
	string str1Lower = str1;
	string str2Lower = str2;

	transform(str1Lower.begin(), str1Lower.end(), str1Lower.begin(), ::tolower);
	transform(str2Lower.begin(), str2Lower.end(), str2Lower.begin(), ::tolower);

	return (str1Lower == str2Lower);
}


