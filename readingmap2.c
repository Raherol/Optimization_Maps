// readingmap2.c
// - counts the number of nodes in the map
// - allocates memory for the nodes and loads the information of each node.
// - loads the edges and shows the information of a particular node.
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <math.h>

#define R 6371

#ifndef M_PI
#define M_PI (3.14159265358979323846)
#endif

typedef struct
{
    unsigned long id; // Node identification
    char *name;
    double lat, lon;      // Node position
    unsigned short nsucc; // Number of node successors; i. e. length of successors
    unsigned long *successors;
    int g;
    double h;
    double f;
} node;

typedef struct
{
    node *nodes;
    unsigned short size;
} queue;

unsigned long searchNode(unsigned long id, node *nodes, unsigned long nnodes);
queue dequeue(queue original_queue);
queue enqueue(queue original_queue, node *nodes, unsigned long index);
double haversine(double lat1, double lon1, double lat2, double lon2);
double toRadians(double degree);

int main(int argc, char *argv[])
{
    clock_t start_time;
    FILE *mapfile;
    unsigned long nnodes;
    char *line = NULL;
    size_t len;

    start_time = clock();

    if (argc < 2)
    {
        mapfile = fopen("andorra.csv", "r");
        printf("Opening map andorra.csv.\n");
    }
    else
    {
        mapfile = fopen(argv[1], "r");
        printf("Opening map %s\n", argv[1]);
    }
    if (mapfile == NULL)
    {
        printf("Error when opening the file\n");
        return 1;
    }
    // count the nodes
    nnodes = 0UL;
    while (getline(&line, &len, mapfile) != -1)
    {
        if (strncmp(line, "node", 4) == 0)
        {
            nnodes++;
        }
    }
    printf("Total number of nodes is %ld\n", nnodes);
    printf("Elapsed time: %f seconds\n", (float)(clock() - start_time) / CLOCKS_PER_SEC);

    rewind(mapfile);

    start_time = clock();
    node *nodes;
    char *tmpline, *field, *ptr;
    unsigned long index = 0;

    nodes = (node *)malloc(nnodes * sizeof(node));
    if (nodes == NULL)
    {
        printf("Error when allocating the memory for the nodes\n");
        return 2;
    }

    while (getline(&line, &len, mapfile) != -1)
    {
        if (strncmp(line, "#", 1) == 0)
            continue;
        tmpline = line; // make a copy of line to tmpline to keep the pointer of line
        field = strsep(&tmpline, "|");
        if (strcmp(field, "node") == 0)
        {
            field = strsep(&tmpline, "|");
            nodes[index].id = strtoul(field, &ptr, 10);
            nodes[index].g = -1;
            field = strsep(&tmpline, "|");
            nodes[index].name = strdup(field);
            for (int i = 0; i < 7; i++)
                field = strsep(&tmpline, "|");
            nodes[index].lat = atof(field);
            field = strsep(&tmpline, "|");
            nodes[index].lon = atof(field);

            nodes[index].nsucc = 0; // start with 0 successors

            index++;
        }
    }
    printf("Assigned data to %ld nodes\n", index);
    printf("Elapsed time: %f seconds\n", (float)(clock() - start_time) / CLOCKS_PER_SEC);
    printf("Last node has:\n id=%lu\n GPS=(%lf,%lf)\n Name=%s\n", nodes[index - 1].id, nodes[index - 1].lat, nodes[index - 1].lon, nodes[index - 1].name);

    rewind(mapfile);

    start_time = clock();
    int oneway;
    unsigned long nedges = 0, origin, dest, originId, destId;
    while (getline(&line, &len, mapfile) != -1)
    {
        if (strncmp(line, "#", 1) == 0)
            continue;
        tmpline = line; // make a copy of line to tmpline to keep the pointer of line
        field = strsep(&tmpline, "|");
        if (strcmp(field, "way") == 0)
        {
            for (int i = 0; i < 7; i++)
                field = strsep(&tmpline, "|"); // skip 7 fields
            if (strcmp(field, "") == 0)
                oneway = 0; // no oneway
            else if (strcmp(field, "oneway") == 0)
                oneway = 1;
            else
                continue;                  // No correct information
            field = strsep(&tmpline, "|"); // skip 1 field
            field = strsep(&tmpline, "|");
            if (field == NULL)
                continue;
            originId = strtoul(field, &ptr, 10);
            origin = searchNode(originId, nodes, nnodes);
            while (1)
            {
                field = strsep(&tmpline, "|");
                if (field == NULL)
                    break;
                destId = strtoul(field, &ptr, 10);
                dest = searchNode(destId, nodes, nnodes);
                if ((origin == nnodes + 1) || (dest == nnodes + 1))
                {
                    originId = destId;
                    origin = dest;
                    continue;
                }
                if (origin == dest)
                    continue;
                // Check if the edge did appear in a previous way
                int newdest = 1;
                for (int i = 0; i < nodes[origin].nsucc; i++)
                    if (nodes[origin].successors[i] == dest)
                    {
                        newdest = 0;
                        break;
                    }
                if (newdest)
                {
                    unsigned short newsize = nodes[origin].nsucc + 1;
                    nodes[origin].successors = realloc(nodes[origin].successors, newsize * sizeof(unsigned long));
                    if (nodes[origin].successors == NULL)
                    {
                        // Handle reallocation failure
                        fprintf(stderr, "Memory allocation failed.\n");
                        exit(EXIT_FAILURE);
                    }
                    nodes[origin].successors[nodes[origin].nsucc] = dest;
                    nodes[origin].nsucc++;
                    nedges++;
                }
                if (!oneway)
                {
                    // Check if the edge did appear in a previous way
                    int newor = 1;
                    for (int i = 0; i < nodes[dest].nsucc; i++)
                        if (nodes[dest].successors[i] == origin)
                        {
                            newor = 0;
                            break;
                        }
                    if (newor)
                    {
                        unsigned short newsize = nodes[dest].nsucc + 1;
                        nodes[dest].successors = realloc(nodes[dest].successors, newsize * sizeof(unsigned long));
                        if (nodes[dest].successors == NULL)
                        {
                            // Handle reallocation failure
                            fprintf(stderr, "Memory allocation failed.\n");
                            exit(EXIT_FAILURE);
                        }
                        nodes[dest].successors[nodes[dest].nsucc] = origin;
                        nodes[dest].nsucc++;
                        nedges++;
                    }
                }
                originId = destId;
                origin = dest;
            }
        }
    }

    fclose(mapfile);
    printf("Assigned %ld edges\n", nedges);
    printf("Elapsed time: %f seconds\n", (float)(clock() - start_time) / CLOCKS_PER_SEC);
    // Look for a node with more than 4 successors
    for (unsigned long i = 0; i < nnodes; i++) // print nodes with more than 2 successors
    {
        if (nodes[i].nsucc > 4)
        {
            index = i;
            break;
        }
        /*{
            printf("Node %lu has id=%lu and %u successors\n",i,nodes[i].id, nodes[i].nsucc);
        }*/
    }
    printf("Node %lu has id=%lu and %u successors:\n", index, nodes[index].id, nodes[index].nsucc);
    for (int i = 0; i < nodes[index].nsucc; i++)
        printf("  Node %lu with id %lu.\n", nodes[index].successors[i], nodes[nodes[index].successors[i]].id);

    node origin_node, target_node;
    int origin_index, target_index;

    // We take the origin and target nodes for the A* algorithm
    for (int i = 0; i < nnodes; i++)
    {
        if (nodes[i].id == atoi(argv[2]))
        {
            nodes[i].g = 0;
            origin_node = nodes[i];
            origin_index = i;
        }
        if (nodes[i].id == atoi(argv[3]))
        {
            target_node = nodes[i];
            target_index = i;
        }
    }

    // We create the queue
    queue priorityqueue;
    priorityqueue.size = 1;
    priorityqueue.nodes = (node *)malloc(sizeof(node));
    priorityqueue.nodes[0] = origin_node;

    // A* algorithm begins
    node current_node;
    unsigned long succ_index;

    while (priorityqueue.size != 0)
    {
        current_node = priorityqueue.nodes[0];
        priorityqueue = dequeue(priorityqueue); // The first element of the queue is taken out

        if (current_node.id == target_node.id)
        {
            break; // We finish if this node is the target one
        }

        if (current_node.nsucc == 0)
        {
            continue; // We skip if it does not have any successors
        }
        else
        {
            for (int i = 0; i < current_node.nsucc; i++) // For every successor
            {
                succ_index = current_node.successors[i];
                if (nodes[succ_index].g == -1) // If it has not been visited we add it to the priority queue
                {
                    nodes[succ_index].g = current_node.g + 1;
                    nodes[succ_index].h = sqrt((nodes[succ_index].lat - target_node.lat) * (nodes[succ_index].lat - target_node.lat) + (nodes[succ_index].lon - target_node.lon) * (nodes[succ_index].lon - target_node.lon));
                    nodes[succ_index].f = nodes[succ_index].g + nodes[succ_index].h;
                    priorityqueue = enqueue(priorityqueue, nodes, succ_index);
                }
            }
        }
    }

    printf("Path was started from: %lu and depth %d\n", nodes[origin_index].id, nodes[origin_index].g);
    printf("Path arrived at: %lu and depth %d\n", nodes[target_index].id, nodes[target_index].g);

    node *finalpath;
    finalpath = (node *)malloc((nodes[target_index].g + 1) * sizeof(node));
    finalpath[nodes[target_index].g] = nodes[target_index];
    for (int i = nodes[target_index].g - 1; i >= 0; i--)
    {
        for (int j = 0; j < nnodes; j++)
        {
            if (nodes[j].g == i)
            {
                for (int k = 0; k < nodes[j].nsucc; k++)
                {
                    if (nodes[nodes[j].successors[k]].id == finalpath[i + 1].id)
                    {
                        finalpath[i] = nodes[j];
                    }
                }
            }
        }
    }

    double total_distance = 0;
    for (int i = 0; i <= nodes[target_index].g; i++)
    {
        if (i != 0)
        {
            total_distance += haversine(finalpath[i].lat, finalpath[i].lon, finalpath[i - 1].lat, finalpath[i - 1].lon);
        }
    }

    FILE *pathtxt;

    pathtxt = fopen("finalpath.txt", "w");

    fprintf(pathtxt, "# Distance from %d to %d: %lf meters.\n", atoi(argv[2]), atoi(argv[3]), total_distance);
    fprintf(pathtxt, "# Optimal path:\n");

    double cumulative_distance = 0;
    for (int i = 0; i <= nodes[target_index].g; i++)
    {
        if (i != 0)
        {
            cumulative_distance += haversine(finalpath[i].lat, finalpath[i].lon, finalpath[i - 1].lat, finalpath[i - 1].lon);
        }
        fprintf(pathtxt, "Id = %lu | %lf | %lf | Dist = %lf\n", finalpath[i].id, finalpath[i].lat, finalpath[i].lon, cumulative_distance);
    }

    fclose(pathtxt);

    return 0;
}

/**
 * Implementation of the A* algorithm
 * TODO: Prepare binary files
 **/

queue enqueue(queue original_queue, node *nodes, unsigned long index)
{
    original_queue.size++;
    original_queue.nodes = realloc(original_queue.nodes, original_queue.size * sizeof(node));
    if (original_queue.size != 1)
    {
        for (int i = 0; i < original_queue.size; i++)
        {
            if (i == original_queue.size - 1)
            {
                original_queue.nodes[i] = nodes[index];
            }
            if (nodes[index].f < original_queue.nodes[i].f)
            {
                for (int j = original_queue.size - 1; j > i; j--)
                {
                    original_queue.nodes[j] = original_queue.nodes[j - 1];
                }
                original_queue.nodes[i] = nodes[index];
                break;
            }
        }
    }
    else
    {
        original_queue.nodes[0] = nodes[index];
    }
    return original_queue;
}

queue dequeue(queue original_queue)
{
    queue temp_queue;
    temp_queue.size = original_queue.size - 1;
    temp_queue.nodes = (node *)malloc(temp_queue.size * sizeof(node));
    for (int i = 1; i < original_queue.size; i++)
    {
        temp_queue.nodes[i - 1] = original_queue.nodes[i];
    }
    free(original_queue.nodes);
    original_queue.nodes = (node *)malloc(temp_queue.size * sizeof(node));
    for (int i = 0; i < temp_queue.size; i++)
    {
        original_queue.nodes[i] = temp_queue.nodes[i];
    }
    original_queue.size = temp_queue.size;
    free(temp_queue.nodes);
    return original_queue;
}

unsigned long searchNode(unsigned long id, node *nodes, unsigned long nnodes)
{
    // we know that the nodes where numrically ordered by id, so we can do a binary search.
    unsigned long l = 0, r = nnodes - 1, m;
    while (l <= r)
    {
        m = l + (r - l) / 2;
        if (nodes[m].id == id)
            return m;
        if (nodes[m].id < id)
            l = m + 1;
        else
            r = m - 1;
    }

    // id not found, we return nnodes+1
    return nnodes + 1;
}

double toRadians(double degree)
{
    return degree * M_PI / 180.0;
}

// Haversine function to calculate distance between two points
double haversine(double lat1, double lon1, double lat2, double lon2)
{
    // Convert latitude and longitude from degrees to radians
    lat1 = toRadians(lat1);
    lon1 = toRadians(lon1);
    lat2 = toRadians(lat2);
    lon2 = toRadians(lon2);

    // Calculate differences in coordinates
    double dlat = lat2 - lat1;
    double dlon = lon2 - lon1;

    // Haversine formula
    double a = pow(sin(dlat / 2), 2) + cos(lat1) * cos(lat2) * pow(sin(dlon / 2), 2);
    double c = 2 * atan2(sqrt(a), sqrt(1 - a));

    // Calculate the distance
    double distance = R * c * 1000;

    return distance;
}