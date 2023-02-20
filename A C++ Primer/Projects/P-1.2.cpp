// C++ A Primer: Projects : P-1.2
/*
		Write a C++ program that, when given a starting day (Sunday through
		 Saturday) as a string, and a four-digit year, prints a calendar for that year.
		 Each month should contain the name of the month, centered over the dates
		 for that month and a line containing the names of the days of the week,
		 running from Sunday to Saturday. Each week should be printed on a sep-
		 arate line. Be careful to check for a leap year
*/

#include<iostream>
#include<iomanip>



int dayOfWeek(int, int, int);
int numberOfDays(int, int);
void DisplayCalendar(int);


int main()
{
    int year;
    std::cout << "Enter the year: ",
        std::cin >> year;

    std::cout << std::endl;
    DisplayCalendar(year);

    return 0;
}

int dayOfWeek(int day, int month, int year)
{
    int monthOffsets[]{ 0, 3, 2, 5, 0, 3, 5, 1,4, 6, 2, 4 };
    if (month < 3) { --year; }

    return (year + (year / 4) - (year / 100) +
            (year / 400) + monthOffsets[month - 1] + day) % 7;
}

int numberOfDays(int month, int year)
{
    switch (month)
    {
    case 1: // January
    case 3: // March
    case 5: // May
    case 7: // July
    case 8: // August
    case 10: // October
    case 12: return 31; // December

    case 4: // April, 
    case 6: // June,
    case 9: // September
    case 11: return 30; // November

    case 2: // February 
        if (not(year % 400) || (not(year % 4) && (year % 100))) // leap
            return 29;

        else// non-leap
            return 28;

    default:
        return 0;
    }
}

void DisplayCalendar(int year)
{
    std::cout << std::setw(23) << " Calendar " << year << "\n\n";

    const char* months[12]{
        "January", "February", "March",
        "April", "May", "June",
        "July", "August", "September",
        "October", "November", "December"
    };

    int currentDay{ dayOfWeek(1, 1, year) };


    for (int month{ 1 }; month < 13; month++)
    {
        int days = numberOfDays(month, year);

        // Print The current month Name
        std::cout << "\t\t" << months[month -1] << std::endl;

        // Print the Columns
        std::cout << "  Sun  Mon  Tue  Wed  Thu  Fri  Sat" << std::endl;

        // Print spaces 
        for (int j{ 0 }; j < currentDay; j++) { std::cout << "     "; }

        //Print days of the Week
        for (int day{ 1 }; day <= days; day++)
        {
            std::cout << std::setw(5) << day ;

            if (++currentDay > 6)
            {
                currentDay = 0;
                std::cout << std::endl;
            }
        }
      
        std::cout << "\n\n";
    }
    return;
}