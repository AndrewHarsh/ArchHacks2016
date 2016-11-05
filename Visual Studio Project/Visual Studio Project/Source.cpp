#include "Network.h"


struct Ping
{
    int ID;
    int RSSi;
};

struct Distance
{
    int ID;
    Ping Beacon;
};

typedef unsigned char byte;

int Serialize(Distance distance, byte* array, int length)
{
    if (length < sizeof(Distance))
    {
        return 0;
    }

    for (int i = 0; i < sizeof(Distance); i++)
    {
        array[i] = ((byte*)(&distance))[i];
    }

    return sizeof(Distance);
}

int Deserialize(byte* array, int length, Distance* distance)
{
    if (length < sizeof(Distance))
    {
        return 0;
    }

    for (int i = 0; i < sizeof(Distance); i++)
    {
        array[i] = ((byte*)(&distance))[i];
    }

    return sizeof(Distance);
}

int main()
{
    Connection_t connection;

    connection.Listen(10000);
    while (true)
    {
        // Get all connected devices
        std::vector<int> ConnectionIndexes = connection.GetIndexes();
        for (int i = 0; i < ConnectionIndexes.size(); i++)
        {
            Distance recieved;
            while (connection.Recv(ConnectionIndexes[i], (char*)&recieved, sizeof(Distance)) > 0)
            {
                printf("ConnectionID: %d\n\tID1: %d\n\tID2: %d\n\nRSSi: %d\n", ConnectionIndexes[i], recieved.Beacon.ID, recieved.ID, recieved.Beacon.RSSi);
            }
        }
    }

    return 0;
}