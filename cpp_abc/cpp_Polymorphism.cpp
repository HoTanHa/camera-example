//Tinh Ke thua + Tinh Da hinh

#include <iostream>
using namespace std;

class Pet
{
protected:
	string Name;

public:
	Pet(string n)
	{
		Name = n;
		cout << "Pet constructor...." << Name << endl;
		MakeSound();
	}
	virtual void MakeSound(void)
	{
		cout << Name << " the Pet say: shh.. shh.." << endl;
	}
	void Wakeup(void)
	{
		cout << "Pet wake up: " << Name << endl;
		MakeSound();
	}
};

class Cat : public Pet
{
private:
	/* data */
public:
	Cat(string n) : Pet(n) {}
	void MakeSound(void)
	{
		cout << Name << " the Cat say: Meow.. Meow.." << endl;
	}
};
class Dog : public Pet
{
private:
	/* data */
public:
	Dog(string n) : Pet(n) {}
	void MakeSound()
	{
		cout << Name << " the Dog say: Woof.. woof.." << endl;
	}
};

int main(void)
{
	Pet *a_pet, *a_pet1, *a_pet2;
	Cat *a_cat;
	Dog *a_dog;

	a_pet = new Pet("APetty");
	a_pet1 = a_cat = new Cat("Tom");
	a_pet2 = a_dog = new Dog("Subidoo");
	cout << endl;
	a_pet->MakeSound();
	a_pet1->MakeSound();
	a_pet2->MakeSound();
	cout << endl;

	a_pet->Wakeup();
	a_cat->Wakeup();
	a_dog->Wakeup();

	delete a_pet;
	delete a_pet1;
	delete a_pet2;
	delete a_cat;
	delete a_dog;

	return 0;
}
