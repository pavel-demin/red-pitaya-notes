#pragma once

using namespace System;
using namespace System::ComponentModel;
using namespace System::Collections;
using namespace System::Windows::Forms;
using namespace System::Data;
using namespace System::Drawing;


namespace ExtIO_RedPitaya {

	/// <summary>
	/// Summary for GUI
	///
	/// WARNING: If you change the name of this class, you will need to change the
	///          'Resource File Name' property for the managed resource compiler tool
	///          associated with all .resx files this class depends on.  Otherwise,
	///          the designers will not be able to interact properly with localized
	///          resources associated with this form.
	/// </summary>
	public ref class GUI : public System::Windows::Forms::Form
	{
	public:
		GUI(void)
		{
			InitializeComponent();
			//
			//TODO: Add the constructor code here
			//
		}

	protected:
		/// <summary>
		/// Clean up any resources being used.
		/// </summary>
		~GUI()
		{
			if (components)
			{
				delete components;
			}
		}
	private: System::Windows::Forms::Label^  label1;
	public: System::Windows::Forms::TextBox^  host;

	private: System::Windows::Forms::Label^  label2;
	public: System::Windows::Forms::NumericUpDown^  port;


	private:
		/// <summary>
		/// Required designer variable.
		/// </summary>
		System::ComponentModel::Container ^components;

#pragma region Windows Form Designer generated code
		/// <summary>
		/// Required method for Designer support - do not modify
		/// the contents of this method with the code editor.
		/// </summary>
		void InitializeComponent(void)
		{
			this->label1 = (gcnew System::Windows::Forms::Label());
			this->host = (gcnew System::Windows::Forms::TextBox());
			this->label2 = (gcnew System::Windows::Forms::Label());
			this->port = (gcnew System::Windows::Forms::NumericUpDown());
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^  >(this->port))->BeginInit();
			this->SuspendLayout();
			// 
			// label1
			// 
			this->label1->AutoSize = true;
			this->label1->Location = System::Drawing::Point(12, 15);
			this->label1->Name = L"label1";
			this->label1->Size = System::Drawing::Size(29, 13);
			this->label1->TabIndex = 0;
			this->label1->Text = L"Host";
			// 
			// host
			// 
			this->host->Location = System::Drawing::Point(48, 12);
			this->host->Name = L"host";
			this->host->Size = System::Drawing::Size(104, 20);
			this->host->TabIndex = 1;
			this->host->Text = L"192.168.1.4";
			// 
			// label2
			// 
			this->label2->AutoSize = true;
			this->label2->Location = System::Drawing::Point(12, 41);
			this->label2->Name = L"label2";
			this->label2->Size = System::Drawing::Size(26, 13);
			this->label2->TabIndex = 2;
			this->label2->Text = L"Port";
			// 
			// port
			// 
			this->port->Location = System::Drawing::Point(48, 39);
			this->port->Maximum = System::Decimal(gcnew cli::array< System::Int32 >(4) {65535, 0, 0, 0});
			this->port->Minimum = System::Decimal(gcnew cli::array< System::Int32 >(4) {1000, 0, 0, 0});
			this->port->Name = L"port";
			this->port->Size = System::Drawing::Size(104, 20);
			this->port->TabIndex = 3;
			this->port->Value = System::Decimal(gcnew cli::array< System::Int32 >(4) {1001, 0, 0, 0});
			// 
			// GUI
			// 
			this->AutoScaleDimensions = System::Drawing::SizeF(6, 13);
			this->AutoScaleMode = System::Windows::Forms::AutoScaleMode::Font;
			this->ClientSize = System::Drawing::Size(164, 72);
			this->Controls->Add(this->port);
			this->Controls->Add(this->label2);
			this->Controls->Add(this->host);
			this->Controls->Add(this->label1);
			this->Name = L"GUI";
			this->Text = L"GUI";
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^  >(this->port))->EndInit();
			this->ResumeLayout(false);
			this->PerformLayout();

		}
#pragma endregion
	};
}
