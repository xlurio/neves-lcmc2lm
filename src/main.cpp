#include <mcc2lm/constants.hpp>

#define MCC2LM_LOG_LEVEL mcc2lm::LogLevel::INFO

#include <cstdlib>
void initialize_database_schema();
void process_all_files();

int main()
{
    initialize_database_schema();
    process_all_files();

    return EXIT_SUCCESS;
}
