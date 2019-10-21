import {mount} from '@vue/test-utils'
import Form from '../../src/components/Form'
import Input from '../../src/components/Input'


describe('Form', () => {
    let cmp;
    beforeEach(() => {
        cmp = mount(Form, {
            propsData: {
                value: {
                    test1: "foo"
                }
            },
            slots: {
                default: {
                    render: (h) => {
                        return h('div',
                            [
                                h(Input, {props: {name: 'test1', type: "text"}}),
                                h(Input, {props: {name: 'test2', type: "select", options: ['0', '1', '2', '3']}}),
                                h(Input, {props: {name: 'test3', type: "switch"}})
                            ]
                        )
                    }
                }
            }
        });
    });

    it('renders the inputs', () => {
        const form = cmp.find(Form);
        expect(form.findAll(Input).length).toBe(3)
    });


    it('should uses the initial value', () => {
        const form = cmp.find(Form);

        expect(form.vm.values).toMatchObject({test1: "foo"});
    });

    it('should updates its values when input changes', () => {
        const form = cmp.find(Form);
        const inputs = form.findAll(Input);

        const in1 = inputs.at(0).find('input');
        in1.setValue("Hello world");

        const in2 = inputs.at(1).find('select');
        in2.setValue(1);

        const in3 = inputs.at(2).find('input');
        in3.setChecked(true);

        expect(form.vm.values).toMatchObject({test1: "Hello world", test2: 1, test3: true});

        in3.setChecked(false);

        expect(form.vm.values).toMatchObject({test1: "Hello world", test2: 1, test3: false});
    });



    it('should emit input when its values are changed', () => {
        const form = cmp.find(Form);

        const in1 = form.find('input');
        in1.setValue("Hello world");

        form.vm.$emit('input');

        expect(cmp.emitted().input).toBeTruthy();
        expect(cmp.emitted().input[0]).toStrictEqual([{test1: "Hello world"}])
    });
});