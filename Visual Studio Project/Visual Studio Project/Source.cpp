#include "Network.h"



struct Distance
{
    int ID1;
    int ID2;
    int RSSi;
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

    connection.Listen(9000);
    while (true)
    {
        // Get all connected devices
        std::vector<int> ConnectionIndexes = connection.GetIndexes();
        for (int i = 0; i < ConnectionIndexes.size(); i++)
        {
            Distance recieved;
            while (connection.Recv(ConnectionIndexes[i], (char*)&recieved, sizeof(Distance)) > 0)
            {
                printf("ConnectionID: %d\n\tID1: %d\n\tID2: %d\n\tRSSi: %d\n", ConnectionIndexes[i], recieved.ID1, recieved.ID2, recieved.RSSi);
            }
        }
    }

    return 0;
}