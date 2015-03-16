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
	private: System::Windows::Forms::Label^  addrLabel;
	public: System::Windows::Forms::TextBox^  addrValue;

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
			this->addrLabel = (gcnew System::Windows::Forms::Label());
			this->addrValue = (gcnew System::Windows::Forms::TextBox());
			this->SuspendLayout();
			// 
			// addrLabel
			// 
			this->addrLabel->AutoSize = true;
			this->addrLabel->Location = System::Drawing::Point(12, 15);
			this->addrLabel->Name = L"addrLabel";
			this->addrLabel->Size = System::Drawing::Size(57, 13);
			this->addrLabel->TabIndex = 0;
			this->addrLabel->Text = L"IP address";
			// 
			// addrValue
			// 
			this->addrValue->Location = System::Drawing::Point(75, 12);
			this->addrValue->Name = L"addrValue";
			this->addrValue->Size = System::Drawing::Size(107, 20);
			this->addrValue->TabIndex = 1;
			// 
			// GUI
			// 
			this->AutoScaleDimensions = System::Drawing::SizeF(6, 13);
			this->AutoScaleMode = System::Windows::Forms::AutoScaleMode::Font;
			this->ClientSize = System::Drawing::Size(194, 42);
			this->Controls->Add(this->addrValue);
			this->Controls->Add(this->addrLabel);
			this->Name = L"GUI";
			this->Text = L"GUI";
			this->ResumeLayout(false);
			this->PerformLayout();

		}
#pragma endregion
	};
}
