#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <wait.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/mman.h>

#define THREAD_SAYISI 4
#define BUFFER_SIZE 1



void *MsjSifreleme(void* arg);
int Boyut_Hesaplama(char *msj);


char *msjParent;
char sifreliMsj[160];
int sifrelenecekMsjBoyutu;

int main(int argc, char *argv[])
{

	// Parent prosesten child prosese ordinary pipe oluþturmak "tek yönlü"
	int pProses_cProses_pipe[2];
	pid_t pid;
	
	//Pipe yaratýlýrken bir hata oluþursa
	if(pipe(pProses_cProses_pipe))
	{
		perror("pipe (...) ");
		return 0;
	}
	
	pid = fork(); // Çalýþmakta olan prosesin kopyasý oluþur. pid=0 ise child oluþur.
	
	if(pid < 0) // fork() fonksiyonu child oluþturmaz.
	{ 
		printf("\nChild proses oluþamaz..\n");
		return 0;
	}
	
	if(pid == 0)// child proses
	{
		
		printf("Child proses..\n");
		char girilenMsj[160];  //girilenMsj==> parent tarafýndan alýnan
		int proses1 = pProses_cProses_pipe[0]; //proses1 e okuma iþlemi
	
		read(proses1, girilenMsj, 161);
		int girilenMsj_boyutu = Boyut_Hesaplama(girilenMsj); //girilen mesaj uzunluðu
		msjParent = girilenMsj;
		
		

		//4 adet threads id oluþturan kod
		pthread_t threads[THREAD_SAYISI];
		int thread_hata;
		
		long t;
	
		//Girilen mesaj boyutu 4 ün katý olmalýdýr.
		if(girilenMsj_boyutu % THREAD_SAYISI == 0)
		{
		 sifrelenecekMsjBoyutu = girilenMsj_boyutu / THREAD_SAYISI;// 4 thread eþit boyutta mesaj alýr
			for(t=0; t<THREAD_SAYISI; t++)
			{
				//Mesajý þifrelemek için 4 thread oluþur
				printf("Oluþan thread sayýsý %ld\n", t);
				thread_hata = pthread_create(&threads[t], NULL, MsjSifreleme, (void *)t);
				if(thread_hata)
				{
					printf("\n Threads oluþturulurken hata = dizin %ld\n\n", t);
				}
			}
	
			for(t=0; t<THREAD_SAYISI; t++)
			{
				//Çocuðun çýkmasýný bekler
			 	pthread_join(threads[t],NULL);	//fonksiyonu, thread'in sonlanmasýný beklenir.(sonlanmasý gereken thread sonlanana kadar)
			}
			
			//sifreliMsj==> Child proses içerisinde þifrelenmiþ mesaj
				
		}
		else
		{
			printf("\nGelen Mesaj %d'e bölünemiyor\n", THREAD_SAYISI);
			
		}
		
		 
		// Shared Memory kýsmý-Child***********************
		const int BOYUT = 4096;
		const char* sharedMemoryAdi = "SIMGE";
		int shm_fd;
		void* pAdres;
		shm_fd = shm_open(sharedMemoryAdi, O_CREAT | O_RDWR, 0666);
		ftruncate(shm_fd, BOYUT); //shared memory alanýnýn boyutunu istediðimiz deðere ayarlayabiliriz
		pAdres =mmap(0, BOYUT, PROT_WRITE, MAP_SHARED, shm_fd, 0);
		sprintf(pAdres,"%s", sifreliMsj);
		pAdres += Boyut_Hesaplama(sifreliMsj);
		
		
	}
	
	
	else
	{// parent process	
		printf("Parent proses..\n");
		const char msj[] = "Nicin hep birlikte baris, uyum icinde yasamayalim? Hepimiz ayni yildizlara bakiyoruz, ayni gezegenin uzerindeki yol arkadaslariyiz ve ayni gokyuzunde yasiyoruz.";
		char girilenMsj[160];

		int proses2 = pProses_cProses_pipe[1]; // proses2 ye yazma iþlemi
		write(proses2, msj, strlen(msj)+1); // msj deki karakter uzunluðu +1
		wait(NULL);
		
		//Shared Memory kýsmý-Parent********************************************************
		printf("\n Geliyor.. \n");
		const int BOYUT = 4096;
		const char* sharedMemoryAdi = "SIMGE";
		int shm_fd;
		void *ptr;
		shm_fd = shm_open(sharedMemoryAdi, O_RDONLY, 0666);
		ptr = mmap(0, BOYUT, PROT_READ, MAP_SHARED, shm_fd,0);
		
		printf("\n Shared memory ile gelen = %s \n",(char *)ptr);
		
		shm_unlink(sharedMemoryAdi);
	}
	
	return 0;
}


//mesaj þifreleme fonksiyonu
void *MsjSifreleme(void* arg)
{
	int id = (int)arg;
	char karakter;
	
	//Threadler ortak alan kullanabildikleri için globalden mesaja eriþim 
	// parent deki mesaj girilenMsj adýnda adesleniyor
	char *girilenMsj = msjParent;
	
	
	printf("*****Mesaj bölüm boyutu =%d\n", sifrelenecekMsjBoyutu);
	char *msj =(char *)malloc(sifrelenecekMsjBoyutu * sizeof(char));
	if(msj== NULL)
	{
		printf("\nBellek ayýrma baþarýsýz oldu..\n");
		exit(1);
	}
	// gelen id'ye göre mesajýn o bölümünü alacak kod 
	int i=0;
	int a = id * sifrelenecekMsjBoyutu;
	while(i<sifrelenecekMsjBoyutu)
	{
		msj[i] = girilenMsj[a];
		a++; i++;
	}
	printf("->Child Thread Id: %d\n--> Þifrelenecek mesaj bölümü = %s\n",id++ , msj);
	int key =0;
	for(int i=0; msj[i] != '\0'; i++)
	{
		karakter = msj[i];
		if(karakter >= 'a' && karakter <= 'z')
		{
			karakter += key;
			if(karakter > 'z')
			{
				karakter = karakter - 'z' + 'a' -1;
			}
		msj[i] = karakter;
		}
		
		
		else if(karakter >= 'A' && karakter <= 'Z')
		{
			karakter += key;
			if(karakter > 'Z')
			{
				karakter = karakter - 'Z' + 'A' -1;
			}
		msj[i] = karakter;
		}
		
	}
	strcat(sifreliMsj,msj); //sifreliMsj + msj
	
	free(msj); // msj belleðini boþaltýr
	pthread_exit(0); // diziden çýkar.
}

//Mesajýn boyutunu hesaplayan fonksiyon
int Boyut_Hesaplama(char *msj)
{
	int sayac = 0;
	for(int i=0; msj[i] != '\0'; i++)
	{
		sayac++;
	}
	return sayac;
}
