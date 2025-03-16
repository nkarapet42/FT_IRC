#include "Bot.hpp"
#include "Server.hpp"

Bot::Bot() {}
Bot::~Bot() {}

void Bot::botHelp(int clientFd, Server& server, const std::string& line) {
	std::stringstream ss(line);
	std::string cmd;
	ss >> cmd;

	std::string restOfLine;
	std::getline(ss, restOfLine);
    if (!restOfLine.empty() && (restOfLine[restOfLine.length() - 1] >= 9
			&& restOfLine[restOfLine.length() - 1] <= 13)) {
			restOfLine.erase(restOfLine.length() - 1);
	}
	if (!restOfLine.empty()) {
        server.sendErrorMessage(clientFd, "Error: Wrong Syntax.", 461);
		return;
	}

	std::string helpMessage = "Inforamtion about Commands:\n";
	helpMessage += "Note: <> means required, [] means optional.\n";
	helpMessage += "DCC GET <filename> - for accept file transfer\n";
	helpMessage += "DCC SEND <nickname> <filename> - for send file to user\n";
	helpMessage += "INVITE <channel> <nickname> - For Inviteing to the channel\n";
	helpMessage += "JOIN <channel> [password] - Join a channel\n";
	helpMessage += "KICK <channel> <nickname> - Kick a user from a channel\n";
	helpMessage += "NICK <nickname> - Set your nickname\n";
	helpMessage += "\nMODE <channel> <mode> [parameter]\n";
    helpMessage += "* Available modes:\n";
    helpMessage += "  - i : Toggle invite-only mode (only invited users can join).\n";
    helpMessage += "  - t : Toggle topic restriction (only operators can change the topic).\n";
    helpMessage += "  - k <password> : Set or remove a password for the channel.\n";
    helpMessage += "  - o <nickname> : Grant or revoke operator privileges for a user.\n";
    helpMessage += "  - l <limit> : Set a user limit for the channel.\n\n";
	helpMessage += "PRIVMSG <nickname> <message> - Send private message\n";
	helpMessage += "PASS <password> - Pass to join Server\n";
	helpMessage += "PART <channel> - Leave the channel\n";
	helpMessage += "QUIT [message] - Quit the server\n";
	helpMessage += "TOPIC <channel> [topic] - Show the channel Topic\n";
	helpMessage += "USER <username> - Set your username\n";
	helpMessage += "WHO [channel] - Show all users in Server [channel]\n";
	helpMessage += "Bot Commands Information:\n";
	helpMessage += "!HELP - Show this help message\n";
	helpMessage += "!MOTD - Get the message of the day \n";
	helpMessage += "!TIME - Get the current server time\n";
	helpMessage += "!WEATHER - Get a random weather forecast\n";

	server.sendMessage(clientFd, std::string(YELLOW) + helpMessage + std::string(RESET), " ");
}

void Bot::sendMotd(int clientFd, Server& server, const std::string& line) {
	std::stringstream ss(line);
	std::string cmd;
	ss >> cmd;

	std::string restOfLine;
	std::getline(ss, restOfLine);
    if (!restOfLine.empty() && (restOfLine[restOfLine.length() - 1] >= 9
			&& restOfLine[restOfLine.length() - 1] <= 13)) {
        restOfLine.erase(restOfLine.length() - 1);
    }
	if (!restOfLine.empty()) {
        server.sendErrorMessage(clientFd, "Error: The !MOTD command must be used without additional arguments.", 461);
		return;
	}
    time_t now = time(0);
    struct tm *timeInfo = localtime(&now);
    int hour = timeInfo->tm_hour;

    std::string greeting;
    if (hour < 12) {
        greeting = "Good morning! Have a great day ahead!";
    } else if (hour < 18) {
        greeting = "Good afternoon! Keep up the good work!";
    } else {
        greeting = "Good evening! Hope you’re having a good time!";
    }

    std::vector<std::string> quotes;
    quotes.push_back("The only way to do great work is to love what you do. - Steve Jobs");
    quotes.push_back("Success is not final, failure is not fatal: It is the courage to continue that counts. - Winston Churchill");
    quotes.push_back("Life is what happens when you're busy making other plans. - John Lennon");
    quotes.push_back("To be yourself in a world that is constantly trying to make you something else is the greatest accomplishment. - Ralph Waldo Emerson");

    srand(time(0));
    int randomIndex = rand() % quotes.size();

    std::string motd = greeting + "\n\n";
    motd += "Here's a random quote for you:\n";
    motd += "\"" + quotes[randomIndex] + "\"";

    server.sendMessage(clientFd, std::string(PURPLE) + motd + "\n" + std::string(RESET), " ", 372);
}

void Bot::sendTime(int clientFd, Server& server, const std::string& line) {
	std::stringstream ss(line);
	std::string cmd;
	ss >> cmd;

	std::string restOfLine;
	std::getline(ss, restOfLine);
    if (!restOfLine.empty() && (restOfLine[restOfLine.length() - 1] >= 9
			&& restOfLine[restOfLine.length() - 1] <= 13)) {
        restOfLine.erase(restOfLine.length() - 1);
    }
	if (!restOfLine.empty()) {
        server.sendErrorMessage(clientFd, "Error: The !TIME command must be used without additional arguments.", 461);
		return;
	}

    time_t now = time(0);
    std::string timeStr = ctime(&now);
    server.sendMessage(clientFd, std::string(GREEN) + "Current time: " + timeStr + std::string(RESET), " ", 391);
}

static std::string getRandomWeather() {
    std::vector<std::string> weatherTypes;
	weatherTypes.push_back("Clear");
	weatherTypes.push_back("Clouds");
	weatherTypes.push_back("Mist");
	weatherTypes.push_back("Snow");
	weatherTypes.push_back("Rain");

	srand(time(0));
    int randomIndex = rand() % weatherTypes.size();
    return weatherTypes[randomIndex];
}

static std::vector<std::string> getWeatherArt(const std::string& weather) {
    std::vector<std::string> res(5, "");

    if (weather == "Clear") {
        res[0] = "     \\   /     ";
        res[1] = "      .-.      ";
        res[2] = "   ― (   ) ―   ";
        res[3] = "      `-’      ";
        res[4] = "     /   \\     ";
    }
    else if (weather == "Clouds") {
        res[0] = "               ";
        res[1] = "      .--.     ";
        res[2] = "   .-(    ).   ";
        res[3] = "  (  .  )  )   ";
        res[4] = "   ‾‾ ‾‾ ‾‾    ";
    }
    else if (weather == "Mist") {
        res[0] = "               ";
        res[1] = "    -   -   -  ";
        res[2] = "  ‾  -‾  -‾    ";
        res[3] = "   ‾-  ‾-  ‾-  ";
        res[4] = "  ‾   ‾   ‾    ";
    }
    else if (weather == "Snow") {
        res[0] = "      .-.      ";
        res[1] = "     (   ).    ";
        res[2] = "    (__(__)    ";
        res[3] = "    * * * *    ";
        res[4] = "   * * * *     ";
    }
    else if (weather == "Rain") {
        res[0] = "      .-.      ";
        res[1] = "     (   ).    ";
        res[2] = "    (___(__)   ";
        res[3] = "   ‚‘‚‘‚‘‚‘    ";
        res[4] = "   ‚’‚’‚’‚’    ";
    }
    else {
        res[0] = "               ";
        res[1] = "               ";
        res[2] = "               ";
        res[3] = "               ";
        res[4] = "               ";
    }

    return res;
}

void Bot::sendWeather(int clientFd, Server& server, const std::string& line) {
	std::stringstream ss(line);
	std::string cmd, location;
	ss >> cmd >> location;

	if (location.empty()) {
        server.sendErrorMessage(clientFd, "Error: The !WEATHER command must be used with location.\n", 461);
		return;
	}
	std::string restOfLine;
	std::getline(ss, restOfLine);
    if (!restOfLine.empty() && (restOfLine[restOfLine.length() - 1] >= 9
			&& restOfLine[restOfLine.length() - 1] <= 13)) {
        restOfLine.erase(restOfLine.length() - 1);
    }
	if (!restOfLine.empty()) {
        server.sendErrorMessage(clientFd, "Error: The !WEATHER command must be used only with one argument.\n", 461);
		return;
	}

    std::string randomWeather = getRandomWeather();

    std::vector<std::string> weatherArt = getWeatherArt(randomWeather);

    std::string weatherInfo = "Weather Forecast at " + location + " : " + randomWeather + "\n";
    for (std::vector<std::string>::iterator it = weatherArt.begin(); it != weatherArt.end(); ++it) {
		weatherInfo += *it + "\n";
	}
    server.sendMessage(clientFd, weatherInfo,   " ", 251);
}
