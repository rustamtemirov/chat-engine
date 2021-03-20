#include <iostream>
#include <uwebsockets/App.h>

using namespace std;

struct PerSocketData
{
   unsigned int id;
    string name;
};

const string DELIMITER = "::";
const string PRIVATE_MESSAGE = "PRIVATE_MESSAGE" + DELIMITER;
const string SET_NAME = "SET_NAME" + DELIMITER;
const string ONLINE = "ONLINE" + DELIMITER;
const string OFFLINE = "OFFLINE" + DELIMITER;


string online(PerSocketData* userData)
{
    return ONLINE + DELIMITER + to_string(userData->id) + DELIMITER + userData->name;
}

string online(pair<unsigned int, string> userData)
{
    return ONLINE + to_string(userData.first) + DELIMITER + userData.second;
}

string offline(PerSocketData* userData)
{
    return OFFLINE + DELIMITER + to_string(userData->id) + DELIMITER+ userData->name;
}

string parsePrivateMessageId(string event)
{
    string rest = event.substr(PRIVATE_MESSAGE.size());
    int pos = rest.find(DELIMITER);
    return rest.substr(0, pos);
}

string parsePrivateMessage(string event)
{
    string rest = event.substr(PRIVATE_MESSAGE.size());
    int pos = rest.find(DELIMITER);
    return rest.substr(pos + DELIMITER.size());
}

//22, "Message" => "PRIVATE_MESSAGE::22:Message
string createPrivateMessage(string senderId, string message, string senderName)
{ 
    return PRIVATE_MESSAGE + senderId + DELIMITER+ "[" + senderName + "]:" + message;
}
/// SET_NAME :: JOHN => John

string parseUserName(string event)
{
    return event.substr(SET_NAME.size() );
}

bool isPrivateMessage(string event)
{
    return event.find(PRIVATE_MESSAGE) == 0;
}

bool isSetName(string event)
{
    return event.find(SET_NAME) == 0;
}


map<unsigned int, string> userNames;

void setUser(PerSocketData* userData)
{
    userNames[userData->id] = userData -> name;
}

int main()
{

    unsigned int last_user_id = 10;

    uWS::App().ws<PerSocketData>("/*", {
        
        // Settings 
        .idleTimeout = 3600,

        /// Function - Handlers 

        .open = [&last_user_id](auto* ws) {
            /// During connection
            PerSocketData* userData = ws->getUserData();
            userData->name = "UNNAMED";
            userData->id = last_user_id++;

            ws->subscribe("user" + to_string(userData->id));
            ws->subscribe("broadcast");

            /// Tell him about users who are connected
            for (auto entry : userNames)
            {
                string onlineEvent = online(entry);
                ws->send(online(entry), uWS::OpCode::TEXT);
            }
            setUser(userData);
            cout << "New user connected: " << userData->id << endl;
        },
        .message = [](auto* ws, std::string_view EventText, uWS::OpCode opCode) {
            /// During the event
            PerSocketData* userData = ws->getUserData();

            string eventString(EventText);

            if (isPrivateMessage(eventString)) {
                string recieverId = parsePrivateMessageId(eventString);
                string text = parsePrivateMessage(eventString);

                string senderId = to_string(userData->id);

                string eventToSend = createPrivateMessage(senderId, text, userData->name);
                ws->publish("user" + recieverId, eventToSend);

                cout << "User " << senderId << " sent message to" << recieverId << endl;
            }
            if (isSetName(eventString))
            {
                string name = parseUserName(eventString);
                userData->name = name;
                setUser(userData);
                ws->publish("broadcast", online(userData));
                cout << "User" << userData->id << " set their name" << endl;
            }
        },

        .close = [](auto* ws , int code, std::string_view message) {
            /// During the disconnection
            PerSocketData* userData = ws->getUserData();
            ws->publish("broadcast", offline(userData));
            cout << "User disconncected " << userData->id;
            userNames.erase(userData->id);
        }
        
        
        
    }).listen(9001, [](auto* listen_socket) {
            if (listen_socket) {
                std::cout << "Listening on port " << 9001 << std::endl;
            }
    }).run();
}